/*
 * sec_fd_detect.c
 *
 * driver supporting debug functions for Samsung device
 *
 * COPYRIGHT(C) Samsung Electronics Co., Ltd. 2006-2011 All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <asm/siginfo.h>

#define MAX_FD 1024
#define TAG	"[FD_DETECT] "

struct trace_fd_state_struct {
	int last_pid;
	int last_tgid;
	char comm[TASK_COMM_LEN];
	bool last_open;
} trace_fd_state[MAX_FD];

struct kobj_pass_info {
	int fd;
	int last_pid;
	int cur_pid;
	char last_comm[TASK_COMM_LEN];
	char cur_comm[TASK_COMM_LEN];
	char file[DNAME_INLINE_LEN];
};

static bool val = false;
static int sys_trace_pid = -1;
static int fdmon_trace_pid = -1;
static int is_mid_dbg = 0;
static int is_upmode = -1;
module_param_named(sys_pid, sys_trace_pid, int, S_IWUSR | S_IRUGO);
module_param_named(fdmon_pid, fdmon_trace_pid, int, S_IWUSR | S_IRUGO);
module_param_named(dbg_mode, is_mid_dbg, int, S_IWUSR | S_IRUGO);
module_param_named(upload_mode, is_upmode, int, S_IWUSR | S_IRUGO);

static struct device *dev;
//static struct platform_device *pdev;
extern int is_file_epoll(struct file *f);

static int is_system(struct task_struct *gleader)
{
	int ret = false;

	if (gleader != NULL &&
		gleader->pid == sys_trace_pid)
		ret = true;

	return ret;
}

static int is_fdmon(struct task_struct *cur)
{
	int ret = false;

	if (current->group_leader != NULL &&
		current->pid == fdmon_trace_pid)
		ret = true;

	return ret;
}

void save_open_close_fdinfo(int fd, int flag, struct task_struct *cur,
				struct files_struct *files)
{
	if (!is_mid_dbg)
		return;

	if (cur->group_leader != NULL &&
		is_system(cur->group_leader) && cur->files == files) {
		if(fd < MAX_FD) {
			trace_fd_state[fd].last_open = flag;
			trace_fd_state[fd].last_pid = cur->pid;
			trace_fd_state[fd].last_tgid = cur->tgid;
			memcpy(trace_fd_state[fd].comm, cur->comm,
					TASK_COMM_LEN);
		}
	}
}

void send_uevent_msg(struct kobj_pass_info *info)
{
	int i;
	char *event[7];

	event[0] = kasprintf(GFP_KERNEL,
			"FD=%d",info->fd);
	event[1] = kasprintf(GFP_KERNEL,
			"CURPID=%d", info->cur_pid);
	event[2] = kasprintf(GFP_KERNEL,
			"CURCOMM=%s", info->cur_comm);
	event[3] = kasprintf(GFP_KERNEL,
			"LASTPID=%d", info->last_pid);
	event[4] = kasprintf(GFP_KERNEL,
			"LASTCOMM=%s", info->last_comm);
	event[5] = kasprintf(GFP_KERNEL,
			"FILE=%s", info->file);
	event[6] = NULL;

	kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, event);
	printk("[FD_DETECT] sent uevent!\n");

	for (i = 0; i < ARRAY_SIZE(event) - 1; i++)
		kfree(event[i]);
}

void check_fd_invalid_close(int fd, struct task_struct *cur,
				struct files_struct *files, struct file *file)
{
	struct siginfo info;
	struct fd f;
	struct kobj_pass_info pass_info;

	if (!is_mid_dbg)
		return;

	if (cur->group_leader != NULL && is_system(cur->group_leader)
			&& !is_fdmon(cur) && cur->files == files) {
		if (fd < MAX_FD &&
		    is_file_epoll(file) &&
		    trace_fd_state[fd].last_pid != cur->pid &&
		    // rule out when closing a parent's fd by a child
		    trace_fd_state[fd].last_pid != cur->group_leader->pid &&
		    // rule out when closing a child's fd by a parent
		    cur->pid != cur->group_leader->pid &&
		    trace_fd_state[fd].last_tgid == cur->tgid) {
			printk(TAG "Try to close epoll fd [%d] "
				"by pid [%d:%s], file=%s\n",
				fd, cur->pid, cur->comm,
				file->f_path.dentry->d_iname);
			printk(TAG "group leader : %s[%d]\n",
					cur->group_leader->comm,
					cur->group_leader->pid);
			printk(TAG "last accessed pid [%d:%s:%s]\n",
				trace_fd_state[fd].last_pid,
				trace_fd_state[fd].comm,
				trace_fd_state[fd].last_open ? "open":"close");

			pass_info.fd = fd;
			pass_info.last_pid = trace_fd_state[fd].last_pid;
			memcpy(pass_info.last_comm, trace_fd_state[fd].comm,
					TASK_COMM_LEN);
			pass_info.cur_pid = cur->pid;
			memcpy(pass_info.cur_comm, cur->comm, TASK_COMM_LEN);
			memcpy(pass_info.file, file->f_path.dentry->d_iname,
					DNAME_INLINE_LEN);

			if (is_upmode != -1 && !val) {
				info.si_signo = 3;
				info.si_errno = 0;
				info.si_code = SI_USER;
				info.si_pid = task_tgid_vnr(cur->group_leader);

				val = true;

				rcu_read_lock();
				kill_pid_info(3, &info,
					find_vpid(cur->group_leader->pid));
				rcu_read_unlock();

				spin_unlock(&files->file_lock);
				// BUG() should be called after doing fsync
				while (val) {
					msleep(100);
				}
				spin_lock(&files->file_lock);
				BUG();
			} else if (!val) {
				spin_unlock(&files->file_lock);
				if (strlen(pass_info.file) !=  0)
					send_uevent_msg(&pass_info);
				spin_lock(&files->file_lock);
			}
		}
	}

	if (is_upmode != -1 && val &&
		!strncmp(file->f_path.dentry->d_iname, "traces.txt", 10)) {
		spin_unlock(&files->file_lock);

		f = fdget(fd);
		if (f.file) {
			vfs_fsync(f.file, 0);
		}

		spin_lock(&files->file_lock);
		printk(KERN_ERR "[FD_DETECT] fsync done\n");
		// All set up to call a BUG()
		val = false;
	}
}

static const struct file_operations fdc_fops = {
	.owner          =       THIS_MODULE,
};

static int __init sec_fd_detect_init(void)
{
	int rc = 0;
	int fdc_major;
	struct class *fdc_class;

	printk(TAG "%s\n", __func__);

	fdc_major = register_chrdev(0, "invalid_fdc", &fdc_fops);
	if (rc) {
		pr_err(TAG "Cant' register chrdev for fdc.\n");
		goto failed;
	}
		
	fdc_class = class_create(THIS_MODULE, "debug");
	if (IS_ERR(fdc_class)) {
		rc = PTR_ERR(fdc_class);
		pr_err(TAG "Can't create fdc class (%d)\n", rc);
		goto failed;
	}

	dev = device_create(fdc_class, NULL, MKDEV(fdc_major, 0),
				NULL, "invalid_fdc");

	if (IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		pr_err(TAG "Can't create fdc device (%d)\n", rc);
		goto failed;
	}

failed :
	return rc;
}
postcore_initcall(sec_fd_detect_init);

