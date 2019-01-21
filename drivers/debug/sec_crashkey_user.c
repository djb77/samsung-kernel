/*
 * drivers/debug/sec_crashkey_user.c
 *
 * COPYRIGHT(C) 2006-2017 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/spinlock.h>
#include <linux/list_sort.h>
#include <linux/sec_debug.h>

#include "sec_key_notifier.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)		(sizeof(a) / sizeof(a[0]))
#endif

/* Input sequence 9530 */
#define CRASH_COUNT_FIRST 9
#define CRASH_COUNT_SECOND 5
#define CRASH_COUNT_THIRD 3

#define KEY_STATE_DOWN 1
#define KEY_STATE_UP 0

/* #define DEBUG */

#ifdef DEBUG
#define SEC_LOG(fmt, args...) pr_info("%s:%s: " fmt "\n", \
		"sec_user_crash_key", __func__, ##args)
#else
#define SEC_LOG(fmt, args...) do { } while (0)
#endif

struct crash_key {
	unsigned int key_code;
	unsigned int crash_count;
};

struct crash_key user_crash_key_combination[] = {
	{KEY_POWER, CRASH_COUNT_FIRST},
	{KEY_VOLUMEUP, CRASH_COUNT_SECOND},
	{KEY_POWER, CRASH_COUNT_THIRD},
};

struct upload_key_state {
	unsigned int key_code;
	unsigned int state;
};

struct upload_key_state upload_key_states[] = {
	{KEY_VOLUMEDOWN, KEY_STATE_UP},
	{KEY_VOLUMEUP, KEY_STATE_UP},
	{KEY_POWER, KEY_STATE_UP},
	{KEY_HOMEPAGE, KEY_STATE_UP},
};

static unsigned int hold_key = KEY_VOLUMEDOWN;
static unsigned int hold_key_hold = KEY_STATE_UP;
static unsigned int check_count;
static unsigned int check_step;

static void cb_keycrash(void)
{
	emerg_pet_watchdog();
	dump_stack();
	dump_all_task_info();
	dump_cpu_stat();
	panic("User Crash Key");
}

static int is_hold_key(unsigned int code)
{
	return (code == hold_key);
}

static void set_hold_key_hold(int state)
{
	hold_key_hold = state;
}

static unsigned int is_hold_key_hold(void)
{
	return hold_key_hold;
}

static unsigned int get_current_step_key_code(void)
{
	return user_crash_key_combination[check_step].key_code;
}

static int is_key_matched_for_current_step(unsigned int code)
{
	SEC_LOG("%d == %d", code, get_current_step_key_code());
	return (code == get_current_step_key_code());
}

static int is_crash_keys(unsigned int code)
{
	unsigned long i;

	for (i = 0; i < ARRAYSIZE(upload_key_states); i++)
		if (upload_key_states[i].key_code == code)
			return 1;
	return 0;
}

static int get_count_for_next_step(void)
{
	int i;
	int count = 0;

	for (i = 0; i < check_step + 1; i++)
		count += user_crash_key_combination[i].crash_count;
	SEC_LOG("%d", count);
	return count;
}

static int is_reaching_count_for_next_step(void)
{
	 return (check_count == get_count_for_next_step());
}

static int get_count_for_panic(void)
{
	unsigned long i;
	int count = 0;

	for (i = 0; i < ARRAYSIZE(user_crash_key_combination); i++)
		count += user_crash_key_combination[i].crash_count;
	return count - 1;
}

static unsigned int is_key_state_down(unsigned int code)
{
	unsigned long i;

	if (is_hold_key(code))
		return is_hold_key_hold();

	for (i = 0; i < ARRAYSIZE(upload_key_states); i++)
		if (upload_key_states[i].key_code == code)
			return upload_key_states[i].state == KEY_STATE_DOWN;
	/* Do not reach here */
	panic("Invalid Keycode");
}

static void set_key_state_down(unsigned int code)
{
	unsigned long i;

	if (is_hold_key(code))
		set_hold_key_hold(KEY_STATE_DOWN);

	for (i = 0; i < ARRAYSIZE(upload_key_states); i++)
		if (upload_key_states[i].key_code == code)
			upload_key_states[i].state = KEY_STATE_DOWN;
	SEC_LOG("code %d", code);
}

static void set_key_state_up(unsigned int code)
{
	unsigned long i;

	if (is_hold_key(code))
		set_hold_key_hold(KEY_STATE_UP);

	for (i = 0; i < ARRAYSIZE(upload_key_states); i++)
		if (upload_key_states[i].key_code == code)
			upload_key_states[i].state = KEY_STATE_UP;
}

static void increase_step(void)
{
	if (check_step < ARRAYSIZE(user_crash_key_combination))
		check_step++;
	else
		cb_keycrash();
	SEC_LOG("%d", check_step);
}

static void reset_step(void)
{
	check_step = 0;
	SEC_LOG("");
}

static void increase_count(void)
{
	if (check_count < get_count_for_panic())
		check_count++;
	else
	        cb_keycrash();
	SEC_LOG("%d < %d", check_count, get_count_for_panic());
}

static void reset_count(void)
{
	check_count = 0;
	SEC_LOG("");
}

static int sec_user_debug_keyboard_call(struct notifier_block *this,
				unsigned long type, void *data)
{
	struct sec_key_notifier_param *param = data;

	unsigned int code = param->keycode;
	int state = param->down;

	if (!is_crash_keys(code))
		return NOTIFY_DONE;

	if (state == KEY_STATE_DOWN) {
		/* Duplicated input */
		if (is_key_state_down(code))
			return NOTIFY_DONE;
		set_key_state_down(code);

		if (is_hold_key(code)) {
			set_hold_key_hold(KEY_STATE_DOWN);
			return NOTIFY_DONE;
		}
		if (is_hold_key_hold()) {
			if (is_key_matched_for_current_step(code)) {
				increase_count();
			} else {
				pr_info("%s: crash key reset\n", "sec_user_crash_key");
				reset_count();
				reset_step();
			}
			if (is_reaching_count_for_next_step())
				increase_step();
		}

	} else {
		set_key_state_up(code);
		if (is_hold_key(code)) {
			pr_info("%s: crash key reset\n", "sec_user_crash_key");
			set_hold_key_hold(KEY_STATE_UP);
			reset_step();
			reset_count();
		}
	}
	return NOTIFY_OK;
}

static struct notifier_block sec_user_debug_keyboard_notifier = {
	.notifier_call = sec_user_debug_keyboard_call,
};

static int __init sec_debug_init_crash_key_user(void)
{

	if (unlikely(!sec_debug_is_enabled()))
		sec_kn_register_notifier(&sec_user_debug_keyboard_notifier);
	return 0;
}
fs_initcall_sync(sec_debug_init_crash_key_user);

