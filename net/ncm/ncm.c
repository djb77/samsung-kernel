/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 * Network Context Metadata Module[NCM]:Implementation.
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/ip.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/err.h>
#include <linux/netfilter_ipv4.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <net/sock.h>

#include <net/ncm.h>

#define SUCCESS 0

#define FAILURE 1
/* fifo size in elements (bytes) */
#define FIFO_SIZE   1024
#define WAIT_TIMEOUT  10000 /*milliseconds */
/* Lock to maintain orderly insertion of elements into kfifo */
static DEFINE_MUTEX(ncm_lock);

static unsigned int ncm_activated_flag = 1;

static unsigned int ncm_deactivated_flag = 0;

static unsigned int device_open_count = 0;

static struct nf_hook_ops nfho;

static struct workqueue_struct *eWq = 0;

wait_queue_head_t ncm_wq;

static atomic_t isNCMEnabled = ATOMIC_INIT(0);

extern struct knox_socket_metadata knox_socket_metadata;

DECLARE_KFIFO(knox_sock_info, struct knox_socket_metadata, FIFO_SIZE);


/* The function is used to check if ncm feature has been enabled or not; The default value is disabled */
unsigned int check_ncm_flag(void) {
    return atomic_read(&isNCMEnabled);
}
EXPORT_SYMBOL(check_ncm_flag);


/** The funcation is used to chedk if the kfifo is active or not;
 *  If the kfifo is active, then the socket metadata would be inserted into the queue which will be read by the user-space;
 *  By default the kfifo is inactive;
 */
bool kfifo_status(void) {
    bool isKfifoActive = false;
    if(kfifo_initialized(&knox_sock_info)) {
        NCM_LOGD("The fifo queue for ncm was already intialized \n");
        isKfifoActive = true;
    } else {
        NCM_LOGE("The fifo queue for ncm is not intialized \n");
        isKfifoActive = false;
    }
    return isKfifoActive;
}
EXPORT_SYMBOL(kfifo_status);


/** The function is used to insert the socket meta-data into the fifo queue; insertion of data will happen in a seperate kernel thread;
 *  The meta data information will be collected from the context of the process which originates it;
 *  If the kfifo is full, then the kfifo is freed before inserting new meta-data;
 */
void insert_data_kfifo(struct work_struct *pwork) {
    struct knox_socket_metadata *knox_socket_metadata;
    if(mutex_lock_interruptible(&ncm_lock)) {
        NCM_LOGE("inserting data into the kfifo failed due to an interuppt \n");
        goto lock_err;
    }
    knox_socket_metadata = container_of(pwork,struct knox_socket_metadata,work_kfifo);
    if(IS_ERR(knox_socket_metadata)) {
        NCM_LOGE("inserting data into the kfifo failed due to unknown error \n");
        goto err;
    }
    if(kfifo_initialized(&knox_sock_info)) {
        if(kfifo_is_full(&knox_sock_info)) {
           NCM_LOGD("The kfifo is full and need to free it \n");
           kfree(knox_socket_metadata);
        } else {
            kfifo_in(&knox_sock_info, knox_socket_metadata,1);
            kfree(knox_socket_metadata);
        }
    } else {
        kfree(knox_socket_metadata);
    }
    mutex_unlock(&ncm_lock);
    err:
        mutex_unlock(&ncm_lock);
        return;
    lock_err:
        return;
}

/** The function is used to insert the socket meta-data into the kfifo in a seperate kernel thread;
 *  The kernel threads which handles the responsibility of inserting the meta-data into the kfifo is manintained by the workqueue function;
 */
void insert_data_kfifo_kthread(struct knox_socket_metadata* knox_socket_metadata) {
    INIT_WORK(&(knox_socket_metadata->work_kfifo), insert_data_kfifo);
    if (!eWq) {
        NCM_LOGD("ewq..Single Thread created\r\n");
        eWq = create_workqueue("ncmworkqueue");
    }
    if (eWq) {
        queue_work(eWq, &(knox_socket_metadata->work_kfifo));
    }
}
EXPORT_SYMBOL(insert_data_kfifo_kthread);


/* The function is used to check if the caller is system server or not; */
static int is_system_server(void) {
    uid_t uid = current_uid().val;
    switch(uid) {
        case 1000:
            return 1;
        case 0:
            return 1;
        default:
            break;
    }
    return 0;
}

/* The function is used to intialize the kfifo */
static void initialize_kfifo(void) {
    INIT_KFIFO(knox_sock_info);
    if(kfifo_initialized(&knox_sock_info)) {
        NCM_LOGD("The kfifo for knox ncm has been initialized \n");
        init_waitqueue_head(&ncm_wq);
    }
}

/* The function is ued to free the kfifo */
static void free_kfifo(void) {
    if(kfifo_status()) {
        NCM_LOGD("The kfifo for knox ncm which was intialized is freed \n");
        kfifo_free(&knox_sock_info);
    }
}

/* The function is used to update the flag indicating whether the feature has been enabled or not */
static void update_ncm_flag(unsigned int ncmFlag) {
    if(ncmFlag == ncm_activated_flag)
        atomic_set(&isNCMEnabled, ncm_activated_flag);
    else
        atomic_set(&isNCMEnabled, ncm_deactivated_flag);
}


/** The function is used to get the source ip address of the packet and update the srcaddr parameter present in struct knox_socket_metadata
 *  The function is registered in the post-routing chain and it is needed to collect the correct source ip address if iptables is involved
 */
static unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *)) {
    struct iphdr *ip_header;
   __be32 masqueraded_source_ip;
    ip_header = (struct iphdr *)skb_network_header(skb);
    masqueraded_source_ip = ip_header->saddr;
    if(skb->sk) {
        skb->sk->inet_src_masq = masqueraded_source_ip;
    }
    return NF_ACCEPT;
}

/* The fuction registers to listen for packets in the post-routing chain to collect the correct source ip address detail; */
static void registerNetfilterHooks(void) {
    nfho.hook = hook_func;
    nfho.hooknum = NF_INET_POST_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_LAST;
    nf_register_hook(&nfho);
}

/* The function un-registers the netfilter hook */
static void unregisterNetFilterHooks(void) {
    nf_unregister_hook(&nfho);
}


/* The function opens the char device through which the userspace reads the socket meta-data information */
static int ncm_open(struct inode *inode, struct file *file) {
    NCM_LOGD("ncm_open is being called. \n");

    if(!is_system_server()) {
        NCM_LOGE("ncm_open failed:Caller is a non system process with uid %u \n",(current_uid().val));
        return -EACCES;
    }

    if (((file->f_flags & O_ACCMODE) == O_WRONLY) || ((file->f_flags & O_ACCMODE) == O_RDWR)) {
        NCM_LOGE("ncm_open failed:Trying to open in write mode \n");
        return -EACCES;
    }

    if (device_open_count) {
        NCM_LOGE("ncm_open failed:The device is already in open state \n");
        return -EBUSY;
    }

    device_open_count++;

    try_module_get(THIS_MODULE);

    return SUCCESS;
}

static ssize_t ncm_copy_data_user(char __user * buf,size_t count) {
    struct knox_socket_metadata kcm = {0};
    struct knox_user_socket_metadata user_copy = {0};

    unsigned long copied;
    int read = 0;

    if(mutex_lock_interruptible(&ncm_lock)) {
        NCM_LOGE("ncm_copy_data_user failed:Signal interuption \n");
        return 0;
    }
    read = kfifo_out(&knox_sock_info, &kcm,1);
    mutex_unlock(&ncm_lock);
    if(read == 0) {
        return 0;
    }

    user_copy.srcport = kcm.srcport;
    user_copy.dstport = kcm.dstport;
    user_copy.trans_proto = kcm.trans_proto;
    user_copy.knox_sent = kcm.knox_sent;
    user_copy.knox_recv = kcm.knox_recv;
    user_copy.knox_uid = kcm.knox_uid;
    user_copy.knox_pid = kcm.knox_pid;
    user_copy.knox_puid = kcm.knox_puid;
    user_copy.open_time = kcm.open_time;
    user_copy.close_time = kcm.close_time;

    memcpy(user_copy.srcaddr,kcm.srcaddr,sizeof(user_copy.srcaddr));
    memcpy(user_copy.dstaddr,kcm.dstaddr,sizeof(user_copy.dstaddr));

    memcpy(user_copy.process_name,kcm.process_name,sizeof(user_copy.process_name));
    memcpy(user_copy.parent_process_name,kcm.parent_process_name,sizeof(user_copy.parent_process_name));

    memcpy(user_copy.domain_name,kcm.domain_name,sizeof(user_copy.domain_name)-1);

    copied = copy_to_user(buf,&user_copy,sizeof(struct knox_user_socket_metadata));
    return count;
}

/* The function writes the socket meta-data to the user-space */
static ssize_t ncm_read(struct file *file, char __user *buf, size_t count, loff_t *off) {
    if(!is_system_server()) {
        NCM_LOGE("ncm_read failed:Caller is a non system process with uid %u \n",(current_uid().val));
        return -EACCES;
    }
    return ncm_copy_data_user(buf,count);
}

/* The function closes the char device */
static int ncm_close(struct inode *inode, struct file *file) {
    NCM_LOGD("ncm_close is being called \n");
    if(!is_system_server()) {
        NCM_LOGE("ncm_close failed:Caller is a non system process with uid %u \n",(current_uid().val));
        return -EACCES;
    }
    device_open_count--;
    module_put(THIS_MODULE);
    if(!check_ncm_flag())  {
        NCM_LOGD("ncm_close success: The device was already in closed state \n");
        return SUCCESS;
    }
    update_ncm_flag(ncm_deactivated_flag);
    free_kfifo();
    unregisterNetFilterHooks();
    return SUCCESS;
}



/* The function sets the flag which indicates whether the ncm feature needs to be enabled or disabled */
static long ncm_ioctl_evt(struct file *file, unsigned int cmd, unsigned long arg) {
    if(!is_system_server()) {
        NCM_LOGE("ncm_ioctl_evt failed:Caller is a non system process with uid %u \n",(current_uid().val));
        return -EACCES;
    }
    switch(cmd) {
        case NCM_ACTIVATED: {
            NCM_LOGD("ncm_ioctl_evt is being NCM_ACTIVATED with the ioctl command %u \n",cmd);
            if(check_ncm_flag()) return SUCCESS;
            registerNetfilterHooks();
            initialize_kfifo();
            update_ncm_flag(ncm_activated_flag);
            break;
        }
        case NCM_DEACTIVATED: {
            NCM_LOGD("ncm_ioctl_evt is being NCM_DEACTIVATED with the ioctl command %u \n",cmd);
            if(!check_ncm_flag()) return SUCCESS;
            update_ncm_flag(ncm_deactivated_flag);
            free_kfifo();
            unregisterNetFilterHooks();
            break;
        }
        default:
            break;
    }
    return SUCCESS;
}

static unsigned int ncm_poll(struct file *file, poll_table *pt) {
    int mask = 0;
    int ret = 0;
    if (kfifo_is_empty(&knox_sock_info)) {
        ret = wait_event_interruptible_timeout(ncm_wq,!kfifo_is_empty(&knox_sock_info), msecs_to_jiffies(WAIT_TIMEOUT));
        switch(ret) {
            case -ERESTARTSYS:
                mask = -EINTR;
                break;
            case 0:
                mask = 0;
                break;
            case 1:
                mask |= POLLIN | POLLRDNORM;
                break;
            default:
                mask |= POLLIN | POLLRDNORM;
                break;
        }
        return mask;
    } else {
        mask |= POLLIN | POLLRDNORM;
    }
    return mask;
}

static const struct file_operations ncm_fops = {
    .owner          = THIS_MODULE,
    .open           = ncm_open,
    .read           = ncm_read,
    .release        = ncm_close,
    .unlocked_ioctl = ncm_ioctl_evt,
    .compat_ioctl   = ncm_ioctl_evt,
    .poll           = ncm_poll,
};

struct miscdevice ncm_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "ncm_dev",
    .fops = &ncm_fops,
};

static int __init ncm_init(void) {
    int ret;
    ret = misc_register(&ncm_misc_device);
    if (unlikely(ret)) {
        NCM_LOGE("failed to register ncm misc device!\n");
        return ret;
    }
    NCM_LOGD("Network Context Metadata Module: initialized\n");
    return SUCCESS;
}

static void __exit ncm_exit(void) {
    misc_deregister(&ncm_misc_device);
    NCM_LOGD("Network Context Metadata Module: unloaded\n");
}

module_init(ncm_init)
module_exit(ncm_exit)

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Network Context Metadata Module:");
