/*
 * include/linux/muic/ccic_notifier.h
 *
 * header file supporting CCIC notifier call chain information
 *
 * Copyright (C) 2010 Samsung Electronics
 * Seung-Jin Hahn <sjin.hahn@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __CCIC_NOTIFIER_H__
#define __CCIC_NOTIFIER_H__

/* CCIC notifier call sequence,
 * largest priority number device will be called first. */
typedef enum {
	CCIC_NOTIFY_DEV_INITIAL = 0,
	CCIC_NOTIFY_DEV_USB,
	CCIC_NOTIFY_DEV_BATTERY,
	CCIC_NOTIFY_DEV_PDIC,
	CCIC_NOTIFY_DEV_MUIC,
	CCIC_NOTIFY_DEV_CCIC,
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER	
	CCIC_NOTIFY_DEV_MANAGER,
#endif
} ccic_notifier_device_t;

typedef enum {
	CCIC_NOTIFY_ID_INITIAL = 0,
	CCIC_NOTIFY_ID_ATTACH,
	CCIC_NOTIFY_ID_RID,
	CCIC_NOTIFY_ID_USB,
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	CCIC_NOTIFY_ID_POWER_STATUS,
#endif
	CCIC_NOTIFY_ID_WATER,
	CCIC_NOTIFY_ID_VCONN,	
} ccic_notifier_id_t;

typedef struct
{
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t sub1:16;
	uint64_t sub2:16;
	uint64_t sub3:16;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	void *pd;
#endif
} CC_NOTI_TYPEDEF;

/* ID = 1 : Attach */
typedef struct
{
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t attach:16;
	uint64_t rprd:16;
	uint64_t cable_type:16;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	void *pd;
#endif
} CC_NOTI_ATTACH_TYPEDEF;

typedef enum {
	CCIC_NOTIFY_DETACH = 0,
	CCIC_NOTIFY_ATTACH,
} ccic_notifier_attach_t;

typedef enum {
	CCIC_NOTIFY_DEVICE = 0,
	CCIC_NOTIFY_HOST,
} ccic_notifier_attach_rprd_t;

/* ID = 2 : RID */
typedef struct
{
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t rid:16;
	uint64_t sub2:16;
	uint64_t sub3:16;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	void *pd;
#endif
} CC_NOTI_RID_TYPEDEF;

typedef enum {
	RID_UNDEFINED = 0,
	RID_000K,
	RID_001K,
	RID_255K,
	RID_301K,
	RID_523K,
	RID_619K,
	RID_OPEN,
} ccic_notifier_rid_t;

/* ID = 3 : USB status */
typedef struct
{
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t attach:16;
	uint64_t drp:16;
	uint64_t sub3:16;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	void *pd;
#endif
} CC_NOTI_USB_STATUS_TYPEDEF;

typedef enum
{
	USB_STATUS_NOTIFY_DETACH = 0,
	USB_STATUS_NOTIFY_ATTACH_DFP = 1, // Host
	USB_STATUS_NOTIFY_ATTACH_UFP = 2, // Device
	USB_STATUS_NOTIFY_ATTACH_DRP = 3, // Dual role
} USB_STATUS; 

/* TODO:  */
struct ccic_notifier_struct {
	CC_NOTI_TYPEDEF ccic_template;
	struct blocking_notifier_head notifier_call_chain;
};

#define CCIC_NOTIFIER_BLOCK(name)	\
	struct notifier_block (name)

extern int ccic_notifier_notify(CC_NOTI_TYPEDEF *, void *, int);
//extern void ccic_notifier_255K_test(void);

/* ccic notifier register/unregister API
 * for used any where want to receive ccic attached device attach/detach. */
extern int ccic_notifier_register(struct notifier_block *nb,
		notifier_fn_t notifier, ccic_notifier_device_t listener);
extern int ccic_notifier_unregister(struct notifier_block *nb);

extern char CCIC_NOTI_DEST_Print[8][10];
extern char CCIC_NOTI_ID_Print[7][20];
extern char CCIC_NOTI_RID_Print[8][15];
extern char CCIC_NOTI_USB_STATUS_Print[5][20];

#endif /* __CCIC_NOTIFIER_H__ */
