/*
 * TEE Driver
 *
 * Copyright (C) 2016 Samsung Electronics, Inc.
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

#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <five_tee_driver.h>
#include "tee_client_api.h"
#include "five_ta_uuid.h"

#ifdef CONFIG_TEE_DRIVER_DEBUG
#include <linux/uaccess.h>
#endif

static DEFINE_MUTEX(itee_driver_lock);
static char is_initialized;
static TEEC_Context *context;
static TEEC_Session *session;

#define MAX_HASH_LEN 64

struct tci_msg {
	uint8_t hash_algo;
	uint8_t hash[MAX_HASH_LEN];
	uint8_t signature[MAX_HASH_LEN];
	uint16_t label_len;
	uint8_t label[0];
} __attribute__((packed));

#define CMD_SIGN   1
#define CMD_VERIFY 2

static int load_trusted_app(void);

static int thread_handler(void *arg)
{
	int rc;
	struct completion *ta_loaded;

	ta_loaded = (struct completion *)arg;
	rc = load_trusted_app();
	complete(ta_loaded);

	/* Wait for kthread_stop */
	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	set_current_state(TASK_RUNNING);

	return rc;
}

static int initialize_trusted_app(void)
{
	int ret = 0;
	struct task_struct *tsk;
	struct completion ta_loaded;

	if (is_initialized)
		return ret;

	init_completion(&ta_loaded);

	tsk = kthread_run(thread_handler, &ta_loaded, "five_load_trusted_app");
	if (IS_ERR(tsk)) {
		ret = PTR_ERR(tsk);
		pr_err("FIVE: Can't create thread: %d\n", ret);
		return ret;
	}

	wait_for_completion(&ta_loaded);
	ret = kthread_stop(tsk);
	is_initialized = ret == 0 ? 1 : 0;
	pr_info("FIVE: Initialized trusted app ret: %d\n", ret);

	return ret;
}

static int send_cmd(unsigned int cmd,
		enum hash_algo algo,
		const void *hash,
		size_t hash_len,
		const void *label,
		size_t label_len,
		void *signature,
		size_t *signature_len)
{
	TEEC_Operation operation = {};
	TEEC_Result rc;
	uint32_t origin;
	struct tci_msg *msg = NULL;
	size_t msg_len;
	size_t sig_len;

	if (!hash || !hash_len ||
			!signature || !signature_len || !(*signature_len))
		return -EINVAL;

	msg_len = sizeof(*msg) + label_len;
	if (label_len > PAGE_SIZE || msg_len > PAGE_SIZE)
		return -EINVAL;

	switch (algo) {
	case HASH_ALGO_SHA1:
		if (hash_len != SHA1_DIGEST_SIZE)
			return -EINVAL;
		sig_len = SHA1_DIGEST_SIZE;
		break;
	case HASH_ALGO_SHA256:
		if (hash_len != SHA256_DIGEST_SIZE)
			return -EINVAL;
		sig_len = SHA256_DIGEST_SIZE;
		break;
	case HASH_ALGO_SHA512:
		if (hash_len != SHA512_DIGEST_SIZE)
			return -EINVAL;
		sig_len = SHA512_DIGEST_SIZE;
		break;
	default:
		return -EINVAL;
	}

	if (cmd == CMD_SIGN && sig_len > *signature_len)
		return -EINVAL;
	if (cmd == CMD_VERIFY && sig_len != *signature_len)
		return -EINVAL;

	msg = kzalloc(msg_len, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hash_algo = algo;
	memcpy(msg->hash, hash, hash_len);
	msg->label_len = label_len;
	if (label_len)
		memcpy(msg->label, label, label_len);

	if (cmd == CMD_VERIFY) {
		memcpy(msg->signature, signature, sig_len);

		operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
							TEEC_NONE,
							TEEC_NONE, TEEC_NONE);
	} else {
		operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
							TEEC_NONE,
							TEEC_NONE, TEEC_NONE);
	}

	operation.params[0].tmpref.buffer = msg;
	operation.params[0].tmpref.size = msg_len;

	mutex_lock(&itee_driver_lock);
	if (initialize_trusted_app()) {
		mutex_unlock(&itee_driver_lock);
		rc = -EIO;
		goto out;
	}

	rc = TEEC_InvokeCommand(session, cmd, &operation, &origin);

	mutex_unlock(&itee_driver_lock);

	if (rc == TEEC_SUCCESS && origin != TEEC_ORIGIN_TRUSTED_APP)
		rc = -EIO;

	if (rc == TEEC_SUCCESS && cmd == CMD_SIGN) {
		memcpy(signature, msg->signature, sig_len);
		*signature_len = sig_len;
	}

out:
	kzfree(msg);
	return rc;
}

static int verify_hmac(enum hash_algo algo, const void *hash, size_t hash_len,
			const void *label, size_t label_len,
			const void *signature, size_t signature_len)
{
	return send_cmd(CMD_VERIFY, algo,
			hash, hash_len, label, label_len,
					(void *)signature, &signature_len);
}

static int sign_hmac(enum hash_algo algo, const void *hash, size_t hash_len,
			const void *label, size_t label_len,
			void *signature, size_t *signature_len)
{
	return send_cmd(CMD_SIGN, algo,
			hash, hash_len, label, label_len,
						signature, signature_len);
}

static int load_trusted_app(void)
{
	TEEC_Result rc;
	uint32_t origin;

	context = kzalloc(sizeof(*context), GFP_KERNEL);
	if (!context) {
		pr_err("FIVE: Can't allocate context\n");
		goto error;
	}

	rc = TEEC_InitializeContext(NULL, context);
	if (rc) {
		pr_err("FIVE: Can't initialize context rc=0x%x\n", rc);
		goto error;
	}

	session = kzalloc(sizeof(*session), GFP_KERNEL);
	if (!session) {
		pr_err("FIVE: Can't allocate session\n");
		goto error;
	}

	rc = TEEC_OpenSession(context, session,
				&five_ta_uuid, 0, NULL, NULL, &origin);
	if (rc) {
		pr_err("FIVE: Can't open session rc=0x%x origin=0x%x\n",
				rc, origin);
		goto error;
	}

	is_initialized = 1;

	return 0;
error:
	TEEC_CloseSession(session);
	TEEC_FinalizeContext(context);
	kfree(session);
	kfree(context);
	session = NULL;
	context = NULL;

	return -1;
}

static int register_tee_driver(void)
{
	struct five_tee_driver_fns fn = {
		.verify_hmac = verify_hmac,
		.sign_hmac = sign_hmac,
	};
	return register_five_tee_driver(&fn);
}

static void unregister_tee_driver(void)
{
	unregister_five_tee_driver();
	/* Don't close session with TA when       */
	/* tee_integrity_driver has been unloaded */
}

#ifdef CONFIG_TEE_DRIVER_DEBUG
static void unload_trusted_app(void)
{
	is_initialized = 0;
	TEEC_CloseSession(session);
	TEEC_FinalizeContext(context);
	kfree(session);
	kfree(context);
	session = NULL;
	context = NULL;
}

static ssize_t tee_driver_write(
		struct file *file, const char __user *buf,
		size_t count, loff_t *pos)
{
	uint8_t hash[SHA1_DIGEST_SIZE] = {};
	uint8_t signature[SHA1_DIGEST_SIZE * 2] = {};
	char command;
	size_t signature_len = sizeof(signature);

	if (get_user(command, buf))
		return -EFAULT;

	switch (command) {
	case '1':
		pr_info("register_tee_driver: %d\n", register_tee_driver());
		break;
	case '2':
		pr_info("sign_hmac: %d\n", sign_hmac(HASH_ALGO_SHA1, hash,
					sizeof(hash), "label", sizeof("label"),
					signature, &signature_len));
		break;
	case '3':
		pr_info("verify_hmac: %d\n", verify_hmac(HASH_ALGO_SHA1, hash,
					sizeof(hash), "label", sizeof("label"),
					signature, signature_len));
		break;
	case '4':
		pr_info("unregister_tee_driver\n");
		unregister_tee_driver();
		mutex_lock(&itee_driver_lock);
		unload_trusted_app();
		mutex_unlock(&itee_driver_lock);
		break;
	default:
		pr_err("FIVE: %s: unknown cmd: %hhx\n", __func__, command);
		return -EINVAL;
	}

	return count;
}

static const struct file_operations tee_driver_fops = {
	.owner = THIS_MODULE,
	.write = tee_driver_write
};

static int __init init_fs(void)
{
	struct dentry *debug_file = NULL;
	umode_t umode =
		(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	debug_file = debugfs_create_file(
			"integrity_tee_driver", umode, NULL, NULL,
							    &tee_driver_fops);
	if (IS_ERR_OR_NULL(debug_file))
		goto error;

	return 0;
error:
	if (debug_file)
		return -PTR_ERR(debug_file);

	return -EEXIST;
}
#else
static inline int __init init_fs(void)
{
	return 0;
}
#endif

static int __init tee_driver_init(void)
{
	register_tee_driver();
	mutex_init(&itee_driver_lock);
	return init_fs();
}

static void __exit tee_driver_exit(void)
{
	unregister_tee_driver();
}

module_init(tee_driver_init);
module_exit(tee_driver_exit);
