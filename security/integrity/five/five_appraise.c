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
#include <linux/fs.h>
#include <linux/xattr.h>
#include <linux/magic.h>
#include <crypto/hash_info.h>

#include <linux/task_integrity.h>
#include "five.h"
#include "five_audit.h"
#include "five_trace.h"
#include "five_tee_api.h"
#include "five_porting.h"

#define FIVE_RSA_SIGNATURE_MAX_LENGTH (2048/8)

/*
 * five_collect_measurement - collect file measurement
 *
 * Must be called with iint->mutex held.
 *
 * Return 0 on success, error code otherwise.
 */
static int five_collect_measurement(struct file *file, u8 hash_algo,
			     u8 *hash, size_t hash_len)
{
	int result = 0;

	BUG_ON(!file || !hash);

	result = five_calc_file_hash(file, hash_algo, hash, &hash_len);
	if (result) {
		five_audit_err(current, file, "collect_measurement", 0,
				0, "calculate file hash failed", result);
	}
	return result;
}

static int get_integrity_label(struct five_cert *cert,
			void **label_data, size_t *label_len)
{
	int rc = -ENODATA;
	struct five_cert_header *header =
			(struct five_cert_header *)cert->body.header->value;

	if (header && header->signature_type == FIVE_XATTR_HMAC) {
		*label_data = cert->body.label->value;
		*label_len = cert->body.label->length;
		rc = 0;
	}
	return rc;
}

static int get_signature(struct five_cert *cert, void **sig,
		size_t *sig_len)
{
	int rc = -ENODATA;
	struct five_cert_header *header =
			(struct five_cert_header *)cert->body.header->value;

	if (header && header->signature_type == FIVE_XATTR_HMAC) {
		if (cert->signature->length == 0)
			return rc;
		*sig = cert->signature->value;
		*sig_len = cert->signature->length;

		rc = 0;
	}

	return rc;
}

static int update_label(struct integrity_iint_cache *iint,
		const void *label_data, size_t label_len)
{
	struct integrity_label *l;

	if (!label_data)
		return 0;

	l = kmalloc(sizeof(struct integrity_label) + label_len, GFP_NOFS);
	if (l) {
		l->len = label_len;
		memcpy(l->data, label_data, label_len);
		kfree(iint->five_label);
		iint->five_label = l;
	} else {
		return -ENOMEM;
	}

	return 0;
}

static int five_fix_xattr(struct task_struct *task,
			  struct dentry *dentry,
			  struct file *file,
			  void **raw_cert,
			  size_t raw_cert_len,
			  struct integrity_iint_cache *iint,
			  struct integrity_label *label)
{
	int rc = 0;
	u8 hash[FIVE_MAX_DIGEST_SIZE], *hash_file, *sig = NULL;
	size_t hash_len = sizeof(hash), hash_file_len, sig_len;
	void *file_label = label ? label->data : NULL;
	short file_label_len = label ? label->len : 0;
	struct five_cert_body body_cert = {0};
	struct five_cert_header *header;

	BUG_ON(!task || !dentry || !file || !raw_cert || !(*raw_cert) || !iint);

	rc = five_cert_body_fillout(&body_cert, *raw_cert, raw_cert_len);
	if (unlikely(rc))
		return -EINVAL;

	header = (struct five_cert_header *)body_cert.header->value;
	hash_file = body_cert.hash->value;
	hash_file_len = body_cert.hash->length;
	if (unlikely(!header || !hash_file))
		return -EINVAL;

	rc = five_collect_measurement(file, header->hash_algo, hash_file,
				      hash_file_len);
	if (unlikely(rc))
		return rc;

	rc = five_cert_calc_hash(&body_cert, hash, &hash_len);
	if (unlikely(rc))
		return rc;

	sig_len = body_cert.hash->length + file_label_len;

	sig = kzalloc(sig_len, GFP_NOFS);
	if (!sig)
		return -ENOMEM;

	rc = sign_hash(header->hash_algo, hash, hash_len,
		       file_label, file_label_len, sig, &sig_len);

	if (!rc) {
		rc = five_cert_append_signature(raw_cert, &raw_cert_len,
						sig, sig_len);
		if (!rc) {
			int count = 1;

			do {
				rc = __vfs_setxattr_noperm(dentry,
							XATTR_NAME_FIVE,
							*raw_cert,
							raw_cert_len,
							0);
				count--;
			} while (count >= 0 && rc != 0);

			if (!rc) {
				rc = update_label(iint,
						file_label, file_label_len);
				iint->version = file_inode(file)->i_version;
			}
		}
	}

	kfree(sig);

	return rc;
}

static void five_set_cache_status(struct integrity_iint_cache *iint,
					enum integrity_status status)
{
	iint->five_status = status;
}

enum five_file_integrity five_get_cache_status(
					struct integrity_iint_cache *iint)
{
	return iint->five_status;
}

int five_read_xattr(struct dentry *dentry, char **xattr_value)
{
	ssize_t ret;

	ret = vfs_getxattr_alloc(dentry, XATTR_NAME_FIVE, xattr_value,
			0, GFP_NOFS);
	if (ret < 0)
		ret = 0;

	return ret;
}

static bool dmverity_protected(struct file *file)
{
	const char *pathname = NULL;
	char *pathbuf = NULL;
	const char system_prefix[] = "/system/";
	bool res = false;

	pathname = five_d_path(&file->f_path, &pathbuf);

	if (pathname) {
		if (!strncmp(pathname, system_prefix,
						sizeof(system_prefix) - 1))
			res = true;
	}

	if (pathbuf)
		__putname(pathbuf);

	return res;
}

static bool bad_fs(struct inode *inode)
{
	if (inode->i_sb->s_magic != EXT4_SUPER_MAGIC)
		return true;

	return false;
}

/*
 * five_appraise_measurement - appraise file measurement
 *
 * Return 0 on success, error code otherwise
 */
int five_appraise_measurement(struct task_struct *task, int func,
			      struct integrity_iint_cache *iint,
			      struct file *file,
			      const unsigned char *filename,
			      struct five_cert *cert)
{
	static const char op[] = "appraise_data";
	char *cause = "unknown";
	struct dentry *dentry = NULL;
	struct inode *inode = NULL;
	enum five_file_integrity status = FIVE_FILE_UNKNOWN;
	enum task_integrity_value prev_integrity;
	int rc = 0;
	u8 hash[FIVE_MAX_DIGEST_SIZE], *hash_file;
	u8 stored_hash[FIVE_MAX_DIGEST_SIZE];
	size_t hash_len = sizeof(hash), hash_file_len;
	struct five_cert_header *header;

	BUG_ON(!task || !iint || !file || !filename);

	prev_integrity = task_integrity_read(task->integrity);
	dentry = file->f_path.dentry;
	inode = d_backing_inode(dentry);

	if (bad_fs(inode)) {
		status = FIVE_FILE_FAIL;
		cause = "bad-fs";
		rc = -ENOTSUPP;
		goto out;
	}

	if (!cert) {
		cause = "no-cert";
		if (dmverity_protected(file))
			status = FIVE_FILE_DMVERITY;
		goto out;
	}

	header = (struct five_cert_header *)cert->body.header->value;
	hash_file = cert->body.hash->value;
	hash_file_len = cert->body.hash->length;
	if (hash_file_len > sizeof(stored_hash)) {
		cause = "invalid-hash-length";
		rc = -EINVAL;
		goto out;
	}

	memcpy(stored_hash, hash_file, hash_file_len);

	if (unlikely(!header || !hash_file)) {
		cause = "invalid-header";
		rc = -EINVAL;
		goto out;
	}

	rc = five_collect_measurement(file, header->hash_algo, hash_file,
				      hash_file_len);
	if (rc) {
		cause = "failed-calc-hash";
		goto out;
	}

	switch (header->signature_type) {
	case FIVE_XATTR_HMAC: {
		u8 *sig = NULL;
		u8 algo = header->hash_algo;
		void *file_label_data;
		size_t file_label_len, sig_len = 0;

		status = FIVE_FILE_FAIL;

		rc = get_integrity_label(cert, &file_label_data,
					 &file_label_len);
		if (unlikely(rc)) {
			cause = "invalid-label-data";
			break;
		}

		if (unlikely(file_label_len > PAGE_SIZE)) {
			cause = "invalid-label-data";
			break;
		}

		rc = get_signature(cert, (void **)&sig, &sig_len);
		if (unlikely(rc)) {
			cause = "invalid-signature-data";
			break;
		}

		rc = five_cert_calc_hash(&cert->body, hash, &hash_len);
		if (unlikely(rc)) {
			cause = "invalid-calc-cert-hash";
			break;
		}

		rc = verify_hash(algo, hash,
				hash_len,
				file_label_data, file_label_len,
				sig, sig_len);
		if (unlikely(rc)) {
			cause = "invalid-hash";
			if (cert) {
				five_audit_hexinfo(file, "stored hash",
					stored_hash, hash_len);
				five_audit_hexinfo(file, "calculated hash",
					hash, hash_len);
				five_audit_hexinfo(file, "HMAC signature",
					sig, sig_len);
			}
			break;
		}

		rc = update_label(iint, file_label_data, file_label_len);
		if (unlikely(rc)) {
			cause = "invalid-update-label";
			break;
		}

		status = FIVE_FILE_HMAC;

		break;
	}
	case FIVE_XATTR_DIGSIG: {
		status = FIVE_FILE_FAIL;

		rc = five_cert_calc_hash(&cert->body, hash, &hash_len);
		if (unlikely(rc)) {
			cause = "invalid-calc-cert-hash";
			break;
		}

		rc = five_digsig_verify(cert, hash, hash_len);

		if (rc) {
			cause = "invalid-signature";
			if (cert) {
				five_audit_hexinfo(file, "stored hash",
					stored_hash, hash_len);
				five_audit_hexinfo(file, "calculated hash",
					hash, hash_len);
				five_audit_hexinfo(file, "RSA signature",
					cert->signature->value,
					cert->signature->length);
			}
			break;
		}

		status = FIVE_FILE_RSA;

		break;
	}
	default:
		status = FIVE_FILE_FAIL;
		cause = "unknown-five-data";
		break;
	}

out:
	iint->version = file_inode(file)->i_version;
	if (status == FIVE_FILE_FAIL || status == FIVE_FILE_UNKNOWN)
		five_audit_info(task, file, op, prev_integrity,
				prev_integrity, cause, rc);

	five_set_cache_status(iint, status);

	return rc;
}

/*
 * five_update_xattr - update 'security.five' hash value
 */
static int five_update_xattr(struct task_struct *task,
		struct integrity_iint_cache *iint, struct file *file,
		struct integrity_label *label)
{
	struct dentry *dentry;
	enum task_integrity_value tint =
				task_integrity_read(current->integrity);
	int rc = 0;
	uint8_t hash[hash_digest_size[five_hash_algo]];
	uint8_t *raw_cert;
	size_t raw_cert_len;
	struct five_cert_header header = {
			.version = FIVE_CERT_VERSION1,
			.privilege = FIVE_PRIV_DEFAULT,
			.hash_algo = five_hash_algo,
			.signature_type = FIVE_XATTR_HMAC };

	BUG_ON(!task || !iint || !file || !label);

	dentry = file->f_path.dentry;

	/* do not collect and update hash for digital signatures */
	if (iint->five_status == FIVE_FILE_RSA) {
		char dummy[512];

		rc = vfs_getxattr(dentry, XATTR_NAME_FIVE, dummy,
			sizeof(dummy));

		// Check if xattr is exist
		if (rc > 0 || rc != -ENODATA) {
			return -EPERM;
		} else {	// xattr does not exist.
			iint->five_status = FIVE_FILE_UNKNOWN;
			pr_err("FIVE: ERROR: Cache is unsynchronized");
		}
	}

	rc = five_cert_body_alloc(&header, hash, sizeof(hash),
				  label->data, label->len,
				  &raw_cert, &raw_cert_len);
	if (rc)
		return rc;

	if (tint == INTEGRITY_PRELOAD_ALLOW_SIGN
				|| tint == INTEGRITY_MIXED_ALLOW_SIGN
				|| tint == INTEGRITY_DMVERITY_ALLOW_SIGN) {
		rc = five_fix_xattr(task, dentry, file,
				(void **)&raw_cert, raw_cert_len, iint, label);
		if (rc)
			pr_err("FIVE: Can't sign hash: rc=%d\n", rc);
	}

	five_cert_free(raw_cert);

	return rc;
}

/**
 * five_inode_post_setattr - reflect file metadata changes
 * @dentry: pointer to the affected dentry
 *
 * Changes to a dentry's metadata might result in needing to appraise.
 *
 * This function is called from notify_change(), which expects the caller
 * to lock the inode's i_mutex.
 */
void five_inode_post_setattr(struct task_struct *task, struct dentry *dentry)
{
	struct inode *inode = d_backing_inode(dentry);
	struct integrity_iint_cache *iint;

	if (!S_ISREG(inode->i_mode))
		return;

	iint = integrity_iint_find(inode);
	if (iint)
		iint->five_status = FIVE_FILE_UNKNOWN;
}

/*
 * five_protect_xattr - protect 'security.five'
 *
 * Ensure that not just anyone can modify or remove 'security.five'.
 */
static int five_protect_xattr(struct dentry *dentry, const char *xattr_name,
			     const void *xattr_value, size_t xattr_value_len)
{
	if (strcmp(xattr_name, XATTR_NAME_FIVE) == 0) {
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		return 1;
	}
	return 0;
}

static void five_reset_appraise_flags(struct inode *inode)
{
	struct integrity_iint_cache *iint;

	if (!S_ISREG(inode->i_mode))
		return;

	iint = integrity_iint_find(inode);
	if (!iint)
		return;

	iint->five_status = FIVE_FILE_UNKNOWN;
}

int five_inode_setxattr(struct dentry *dentry, const char *xattr_name,
			const void *xattr_value, size_t xattr_value_len)
{
	int result = five_protect_xattr(dentry, xattr_name, xattr_value,
				   xattr_value_len);
	if (result == 1) {
		bool digsig;
		struct five_cert_header *header;
		struct five_cert cert = { {0} };

		result = five_cert_fillout(&cert, xattr_value, xattr_value_len);
		if (result)
			return result;

		header = (struct five_cert_header *)cert.body.header->value;

		if (!xattr_value_len || !header ||
				(header->signature_type >= FIVE_XATTR_END))
			return -EINVAL;

		digsig = (header->signature_type == FIVE_XATTR_DIGSIG);
		if (!digsig)
			return -EPERM;

		five_reset_appraise_flags(d_backing_inode(dentry));
		result = 0;
	}
	return result;
}

int five_inode_removexattr(struct dentry *dentry, const char *xattr_name)
{
	int result;

	result = five_protect_xattr(dentry, xattr_name, NULL, 0);
	if (result == 1) {
		five_reset_appraise_flags(d_backing_inode(dentry));
		result = 0;
	}
	return result;
}

/* Called from do_fcntl */
int five_fcntl_sign(struct file *file, struct integrity_label __user *label)
{
	struct integrity_iint_cache *iint;
	struct inode *inode = file_inode(file);
	struct integrity_label *l = NULL;
	int rc = 0;
	enum task_integrity_value tint =
				    task_integrity_read(current->integrity);

	trace_five_sign_enter(file);

	if (!S_ISREG(inode->i_mode)) {
		trace_five_sign_exit(file,
			FIVE_TRACE_MEASUREMENT_OP_FILTEROUT,
			FIVE_TRACE_FILTEROUT_NONREG);
		return -EINVAL;
	}

	if (tint == INTEGRITY_PRELOAD_ALLOW_SIGN
		|| tint == INTEGRITY_MIXED_ALLOW_SIGN
		|| tint == INTEGRITY_DMVERITY_ALLOW_SIGN) {
		struct integrity_label label_hdr = {0};
		size_t len;

		if (!label)
			return -EINVAL;

		if (unlikely(copy_from_user(&label_hdr,
					    label, sizeof(label_hdr))))
			return -EFAULT;

		if (label_hdr.len > PAGE_SIZE)
			return -EINVAL;

		len = label_hdr.len + sizeof(label_hdr);

		l = kmalloc(len, GFP_NOFS);
		if (!l)
			return -ENOMEM;

		if (unlikely(copy_from_user(l, label, len))) {
			kfree(l);
			return -EFAULT;
		}

		l->len = label_hdr.len;
	} else {
		trace_five_sign_exit(file,
			FIVE_TRACE_MEASUREMENT_OP_FILTEROUT,
			FIVE_TRACE_FILTEROUT_NONEINTEGRITY);
		five_audit_err(current, file, "fcntl_sign", tint, tint,
				"sign:no-perm", -EPERM);
		return -EPERM;
	}

	iint = integrity_inode_get(inode);
	if (!iint) {
		kfree(l);
		return -ENOMEM;
	}

	if (file->f_op && file->f_op->flush)
		if (file->f_op->flush(file, current->files)) {
			kfree(l);
			return -EOPNOTSUPP;
		}

	inode_lock(inode);
	rc = five_update_xattr(current, iint, file, l);
	inode_unlock(inode);

	if (label)
		kfree(l);

	trace_five_sign_exit(file,
				FIVE_TRACE_MEASUREMENT_OP_CALC,
				FIVE_TRACE_MEASUREMENT_TYPE_HMAC);

	return rc;
}
