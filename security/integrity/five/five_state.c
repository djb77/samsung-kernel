/*
 * FIVE State machine
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

#include <linux/task_integrity.h>
#include "five_state.h"

static int is_system_label(struct integrity_label *label)
{
	if (label && label->len == 0)
		return 1; /* system label */

	return 0;
}

static inline int integrity_label_cmp(struct integrity_label *l1,
					struct integrity_label *l2)
{
	return 0;
}

static int verify_or_update_label(struct task_integrity *intg,
		struct integrity_iint_cache *iint)
{
	struct integrity_label *l;
	struct integrity_label *file_label = iint->five_label;
	int rc = 0;

	if (!file_label) /* digsig doesn't have label */
		return 0;

	if (is_system_label(file_label))
		return 0;

	spin_lock(&intg->lock);
	l = intg->label;

	if (l) {
		if (integrity_label_cmp(file_label, l)) {
			rc = -EPERM;
			goto out;
		}
	} else {
		struct integrity_label *new_label;

		new_label = kmalloc(sizeof(file_label->len) + file_label->len,
				GFP_ATOMIC);
		if (!new_label) {
			rc = -ENOMEM;
			goto out;
		}

		new_label->len = file_label->len;
		memcpy(new_label->data, file_label->data, new_label->len);
		intg->label = new_label;
	}

out:
	spin_unlock(&intg->lock);

	return rc;
}

static bool set_first_state(struct integrity_iint_cache *iint,
				struct task_integrity *integrity,
				enum task_integrity_value *new_tint,
				const char **msg)
{
	enum task_integrity_value tint = INTEGRITY_NONE;
	bool trusted_file = iint->five_flags & FIVE_TRUSTED_FILE;

	task_integrity_clear(integrity);

	switch (iint->five_status) {
	case FIVE_FILE_RSA:
		if (trusted_file)
			tint = INTEGRITY_PRELOAD_ALLOW_SIGN;
		else
			tint = INTEGRITY_PRELOAD;
		break;
	case FIVE_FILE_DMVERITY:
		tint = INTEGRITY_DMVERITY;
		break;
	case FIVE_FILE_HMAC:
		tint = INTEGRITY_MIXED;
		break;
	default:
		tint = INTEGRITY_NONE;
		break;
	}

	task_integrity_set(integrity, tint);

	*new_tint = tint;
	if (msg)
		*msg = "bprm-check";

	return true;
}

static bool set_next_state(struct integrity_iint_cache *iint,
			   struct task_integrity *integrity,
			   enum task_integrity_value *new_tint,
			   const char **msg)
{
	bool is_newstate = false;
	enum task_integrity_value source_tint = task_integrity_read(integrity);
	bool has_digsig = (iint->five_status == FIVE_FILE_RSA);
	bool dmv_protected = (iint->five_status == FIVE_FILE_DMVERITY);
	struct integrity_label *label = iint->five_label;
	const char *cause = "unknown";
	enum task_integrity_value state_tint = INTEGRITY_NONE;

	if (has_digsig)
		return is_newstate;

	iint->five_flags &= ~FIVE_TRUSTED_FILE;

	if (verify_or_update_label(integrity, iint)) {
		task_integrity_reset(integrity);
		cause = "mismatch-label";
		state_tint = INTEGRITY_NONE;
		is_newstate = true;
		goto out;
	}

	switch (source_tint) {
	case INTEGRITY_PRELOAD_ALLOW_SIGN:
		if (dmv_protected) {
			task_integrity_cmpxchg(integrity,
					INTEGRITY_PRELOAD_ALLOW_SIGN,
					INTEGRITY_DMVERITY_ALLOW_SIGN);
			cause = "dmverity-allow-sign";
			state_tint = INTEGRITY_DMVERITY_ALLOW_SIGN;
		} else if (is_system_label(label)) {
			task_integrity_cmpxchg(integrity,
					INTEGRITY_PRELOAD_ALLOW_SIGN,
					INTEGRITY_MIXED_ALLOW_SIGN);
			cause = "mixed-allow-sign";
			state_tint = INTEGRITY_MIXED_ALLOW_SIGN;
		} else {
			task_integrity_set_unless(integrity,
					INTEGRITY_MIXED,
					INTEGRITY_NONE);
			cause = "mixed";
			state_tint = INTEGRITY_MIXED;
		}
		is_newstate = true;
		break;
	case INTEGRITY_PRELOAD:
		if (dmv_protected) {
			task_integrity_cmpxchg(integrity,
					INTEGRITY_PRELOAD,
					INTEGRITY_DMVERITY);
			cause = "dmverity";
			state_tint = INTEGRITY_DMVERITY;
		} else {
			task_integrity_set_unless(integrity,
					INTEGRITY_MIXED,
					INTEGRITY_NONE);
			cause = "mixed";
			state_tint = INTEGRITY_MIXED;
		}
		is_newstate = true;
		break;
	case INTEGRITY_MIXED_ALLOW_SIGN:
		if (!dmv_protected && !is_system_label(label)) {
			task_integrity_set_unless(integrity,
					INTEGRITY_MIXED,
					INTEGRITY_NONE);
			cause = "mixed";
			state_tint = INTEGRITY_MIXED;
			is_newstate = true;
		}
		break;
	case INTEGRITY_DMVERITY:
		if (!dmv_protected) {
			task_integrity_set_unless(integrity,
					INTEGRITY_MIXED,
					INTEGRITY_NONE);
			cause = "mixed";
			state_tint = INTEGRITY_MIXED;
			is_newstate = true;
		}
		break;
	case INTEGRITY_DMVERITY_ALLOW_SIGN:
		if (!dmv_protected) {
			if (is_system_label(label)) {
				task_integrity_cmpxchg(integrity,
						INTEGRITY_DMVERITY_ALLOW_SIGN,
						INTEGRITY_MIXED_ALLOW_SIGN);
				cause = "mixed-allow-sign";
				state_tint = INTEGRITY_MIXED_ALLOW_SIGN;
			} else {
				task_integrity_set_unless(integrity,
						INTEGRITY_MIXED,
						INTEGRITY_NONE);
				cause = "mixed";
				state_tint = INTEGRITY_MIXED;
			}
			is_newstate = true;
		}
		break;
	case INTEGRITY_MIXED:
		break;
	case INTEGRITY_NONE:
		break;
	default:
		// Unknown state
		task_integrity_reset(integrity);
		cause = "unknown-source-integrity";
		state_tint = INTEGRITY_NONE;
		is_newstate = true;
	}

out:

	if (is_newstate) {
		*new_tint = state_tint;
		if (msg)
			*msg = cause;
	}

	return is_newstate;
}

bool five_state_proceed(struct integrity_iint_cache *iint,
			struct task_integrity *integrity,
			enum five_hooks fn,
			enum task_integrity_value *new_tint,
			const char **msg)
{
	bool is_newstate;

	if (fn == BPRM_CHECK)
		is_newstate = set_first_state(iint, integrity, new_tint, msg);
	else
		is_newstate = set_next_state(iint, integrity, new_tint, msg);

	return is_newstate;
}
