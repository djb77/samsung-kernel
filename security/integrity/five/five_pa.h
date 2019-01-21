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

#ifndef __LINUX_FIVE_PROCA_H
#define __LINUX_FIVE_PROCA_H

#ifdef CONFIG_FIVE_PA_FEATURE
int fivepa_push_set_xattr_event(struct task_struct *task, struct file *file);
int fivepa_init_signature_wq(void);
void fivepa_fsignature_free(struct file *file);
#else
static inline int fivepa_push_set_xattr_event(struct task_struct *task,
				       struct file *file)
{
	return 0;
}

static inline int fivepa_init_signature_wq(void)
{
	return 0;
}

static inline void fivepa_fsignature_free(struct file *file)
{
}
#endif

#endif // __LINUX_FIVE_PROCA_H
