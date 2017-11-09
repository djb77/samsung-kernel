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

#ifndef NCM_COMMON_H__
#define NCM_COMMON_H__

#include <linux/kernel.h>
#include <linux/inet.h>
#include <linux/sched.h>


/* Struct Socket definition */
struct knox_socket_metadata {
/* The source port of the socket */
    __u16   srcport;
/* The destination port of the socket */
    __u16   dstport;
/* The Transport layer protocol of the socket*/
    __u16   trans_proto;
/* The number of application layer bytes sent by the socket */
    __u64   knox_sent;
/* The number of application layer bytes recieved by the socket */
    __u64   knox_recv;
/* The uid which created the socket */
    uid_t   knox_uid;
/* The pid under which the socket was created */
    pid_t   knox_pid;
/* The parent user id under which the socket was created */
    uid_t	knox_puid;
/* The epoch time at which the socket was opened */
    __u64   open_time;
/* The epoch time at which the socket was closed */
    __u64   close_time;
/* The source address of the socket */
    char srcaddr[INET_ADDRSTRLEN];
/* The destination address of the socket */
    char dstaddr[INET_ADDRSTRLEN];
/* The name of the process which created the socket */
    char process_name[128];
/* The name of the parent process which created the socket */
    char parent_process_name[TASK_COMM_LEN];
/*  The Domain name associated with the ip address of the socket. The size needs to be in sync with the userspace implementation */
    char domain_name[255];
/* The struct defined is responsible for inserting the socket meta-data into kfifo */
    struct work_struct work_kfifo;
};

/* Struct Socket definition */
struct knox_user_socket_metadata {
/* The source port of the socket */
    __u16   srcport;
/* The destination port of the socket */
    __u16   dstport;
/* The Transport layer protocol of the socket*/
    __u16   trans_proto;
/* The number of application layer bytes sent by the socket */
    __u64  knox_sent;
/* The number of application layer bytes recieved by the socket */
    __u64  knox_recv;
/* The uid which created the socket */
    uid_t   knox_uid;
/* The pid under which the socket was created */
    pid_t   knox_pid;
/* The parent user id under which the socket was created */
    uid_t   knox_puid;
/* The epoch time at which the socket was opened */
    __u64   open_time;
/* The epoch time at which the socket was closed */
    __u64   close_time;
/* The source address of the socket */
    char srcaddr[INET_ADDRSTRLEN];
/* The destination address of the socket */
    char dstaddr[INET_ADDRSTRLEN];
/* The name of the process which created the socket */
    char process_name[128];
/* The name of the parent process which created the socket */
    char parent_process_name[TASK_COMM_LEN];
/*  The Domain name associated with the ip address of the socket. The size needs to be in sync with the userspace implementation */
    char domain_name[255];
};

/* The list of function which is being referenced by the af_inet.c class */
extern unsigned int check_ncm_flag(void);
extern bool kfifo_status(void);
extern void insert_data_kfifo_kthread(struct knox_socket_metadata* knox_socket_metadata);

/* Debug */
#define NCM_DEBUG        1
#if NCM_DEBUG
#define NCM_LOGD(...) printk("ncm: "__VA_ARGS__)
#else
#define NCM_LOGD(...)
#endif /* NCM_DEBUG */
#define NCM_LOGE(...) printk("ncm: "__VA_ARGS__)



/* IOCTL definitions*/
#define __NCMIOC    0x120
#define NCM_ACTIVATED       _IO(__NCMIOC, 2)
#define NCM_DEACTIVATED     _IO(__NCMIOC, 4)

#endif
