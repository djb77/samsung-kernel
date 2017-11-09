/* iva_sysfs.c
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Authors:
 *	Ilkwan Kim <ilkwan.kim@samsung.com>
 *	Hoon Choi <hoon98.choi@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/sysfs.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include "iva_ctrl_ioctl.h"
#include "iva_ctrl.h"
#include "iva_mem.h"
#include "iva_ipc_queue.h"

static int iva_sysfs_print(char *buf, size_t buf_sz, const char *fmt, ...)
{
	int	strlen;
	va_list	ap;

	va_start(ap, fmt);
	strlen = vsnprintf(buf, buf_sz, fmt, ap);
	va_end(ap);

	return strlen;
}

#define IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size, format, ...)		\
	do {									\
		tmp_size = iva_sysfs_print(tmp_buf, sizeof(tmp_buf), 		\
						format, ## __VA_ARGS__);	\
		if ((buf_size + tmp_size + 1) > PAGE_SIZE)			\
			goto exit;	/* no space */				\
		strncat(buf, tmp_buf, tmp_size);				\
		buf_size += tmp_size;						\
	} while (0)

static ssize_t iva_mem_show(struct device *dev,
		struct device_attribute *dev_attr, char *buf)
{
	struct iva_dev_data 	*iva = dev_get_drvdata(dev);
	struct iva_mem_map 	*iva_map_node;
	struct list_head	*work_node;
	int 			i = 0;
	ssize_t 		buf_size = 0;
	char			tmp_buf[256];
	int			tmp_size;

	buf[0] = 0;

	mutex_lock(&iva->mem_map_lock);
	IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
			"------------- show iva mapped list (%4d)----------\n",
			iva->map_nr);

	list_for_each(work_node, &iva->mem_map_list) {
		iva_map_node = list_entry(work_node, struct iva_mem_map, node);
		IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
				"[%d] buf_fd:%04d,\tflags:0x%x,\tio_va:0x%08lx,"
				"\tsize:0x%08x, 0x%08x,\tref_cnt:%d\n", i,
				iva_map_node->shared_fd, iva_map_node->flags,
				iva_map_node->io_va,
				iva_map_node->req_size, iva_map_node->act_size,
				iva_mem_map_read_refcnt(iva_map_node));
		i++;
	}

	IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
		"---------------------------------------------------\n");
exit:
	mutex_unlock(&iva->mem_map_lock);

	return buf_size;
}

static ssize_t iva_mem_proc_show(struct device *dev,
		struct device_attribute *dev_attr, char *buf)
{
	struct iva_dev_data 	*iva = dev_get_drvdata(dev);
	struct iva_mem_map 	*iva_map_node;
	struct iva_proc 	*iva_proc_node;
	struct list_head	*work_node_parent;
	int 			i = 0;
	ssize_t 		buf_size = 0;
	char			tmp_buf[256];
	int			tmp_size;

	mutex_lock(&iva->mem_map_lock);
	list_for_each(work_node_parent, &iva->proc_head) {
		iva_proc_node = list_entry(work_node_parent,
				struct iva_proc, proc_node);

		IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
			"------- show iva mapped list of pid(%2d) -------\n",
			iva_proc_node->pid);
		if (hash_empty(iva_proc_node->h_mem_map)) {
			IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
				" (No mem_map in this process) ");
			continue;
		}

		hash_for_each(iva_proc_node->h_mem_map, i, iva_map_node, h_node) {
			IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
				"[%d] buf_fd:%04d,\tflags:0x%x,\tio_va:0x%08lx,"
				"\tsize:0x%08x, 0x%08x\tref_cnt:%d\n", i,
				iva_map_node->shared_fd, iva_map_node->flags,
				iva_map_node->io_va,
				iva_map_node->req_size, iva_map_node->act_size,
				iva_mem_map_read_refcnt(iva_map_node));
			i++;
		}
		IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
			"------------------------------------------------\n");
	}
exit:
	mutex_unlock(&iva->mem_map_lock);
	return buf_size;
}


static ssize_t iva_ipcq_show_trans(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iva_dev_data 	*iva = dev_get_drvdata(dev);
	struct iva_ipcq_stat	*ipcq_stat = &iva->mcu_ipcq.ipcq_stat;
	struct list_head	*work_node;
	struct iva_ipcq_trans	*trans_item;
	unsigned long		rem_nsec;
	u64			ts_ns;
	int			i = 0;
	ssize_t 		buf_size = 0;
	char			tmp_buf[256];
	int			tmp_size;

	IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
			"------- ipc queue transactions"
			"(max nr:%d, nr: %d) -------\n",
			ipcq_stat->trans_nr_max, ipcq_stat->trans_nr);

	list_for_each(work_node, &ipcq_stat->trans_head) {
		trans_item = list_entry(work_node, struct iva_ipcq_trans, node);

		ts_ns = trans_item->ts_nsec;
		rem_nsec = do_div(ts_ns, 1000000000);
		IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
				"(%03d) [%05lu.%06lu] ", i,
				(unsigned long) ts_ns,
				rem_nsec / 1000);

		/* TO DO */
		if (trans_item->send) {
			struct ipc_cmd_param *ipc_msg_c = &trans_item->cmd_param;

			IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
					"--> ");

			switch (ipc_msg_c->ipc_cmd_type) {
			case 5:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"SCHEDULE_TABLE\n");
			#ifdef CONFIG_IVA_RT_PARAM
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tGST: 0x%08X"
						"\tsize(0x%X, %d)\n",
						ipc_msg_c->data.p.p_g_sched_t,
						ipc_msg_c->data.p.g_sched_t_size,
						ipc_msg_c->data.p.g_sched_t_size);

				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tPRM: 0x%08X"
						"\tsize(0x%X, %d)\n",
						ipc_msg_c->data.p.p_rt_param_t,
						ipc_msg_c->data.p.rt_param_size,
						ipc_msg_c->data.p.rt_param_size);
			#endif
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTSS: 0x%08X"
						"\tsize(0x%X, %d)\n",
						ipc_msg_c->data.p.p_rt_table,
						ipc_msg_c->data.p.rt_table_size,
						ipc_msg_c->data.p.rt_table_size);

				break;
			case 10:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"FRAME_START\n");
				break;
			case 12:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"FRAME_DONE\n");
				break;
			case 11:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"NODE_START\n");
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTABLE ID: 0x%08X"
						"\tGUID: 0x%08X\n",
						ipc_msg_c->data.m.table_id,
						ipc_msg_c->data.m.guid);
				break;
			case 9:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"EVICT\n");
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTABLE ID: 0x%08X\n",
						ipc_msg_c->data.m.table_id);
				break;
			case 7:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"UPDATE_SCHEDULE_TABLE\n");
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTSS: 0x%08X"
						"\tsize(0x%X, %d)\n",
						ipc_msg_c->data.p.p_rt_table,
						ipc_msg_c->data.p.rt_table_size,
						ipc_msg_c->data.p.rt_table_size);
				break;
			default:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"cmd type(%d)\n",
						ipc_msg_c->ipc_cmd_type);
			}
		} else {
			struct ipc_res_param *ipc_msg_r = &trans_item->res_param;
			IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
					"<-- ");
			switch (ipc_msg_r->ipc_cmd_type) {
			case 6:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"SCHEDULE_TABLE_ACK\n");
				break;
			case 12:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"FRAME_DONE\n");
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTABLE ID(0x%08X)"
						"\tGUID(0x%08X)\n",
					ipc_msg_r->data.m.table_id,
					ipc_msg_r->data.m.guid);
				break;
			case 13:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"NODE_DONE\n");
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"\t\tTABLE ID(0x%08X)"
						"\tGUID(0x%08X)\n",
					ipc_msg_r->data.m.table_id,
					ipc_msg_r->data.m.guid);
				break;
			case 9:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"EVICT\n");
				break;
			case 2:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"DEINIT\n");
				break;
			case 8:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"UPDATE_SCHEDULE_TABLE_ACK\n");
				break;
			default:
				IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
						"cmd type(%d)\n",
						ipc_msg_r->ipc_cmd_type);
			}
		}
		i++;
	}
	IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
			"---------------------------------------"
			"---------------\n");
exit:
	return buf_size;
}

static ssize_t iva_ipcq_set_trans(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
#define MAX_TRANSACTION_SAVA_NR		(40)
	struct iva_dev_data 	*iva = dev_get_drvdata(dev);
	struct iva_ipcq_stat	*ipcq_stat = &iva->mcu_ipcq.ipcq_stat;
	struct list_head	*work_node;
	struct list_head	*tmp_node;
	struct iva_ipcq_trans	*trans_item;
	int		value;
	int		err;

	err = kstrtoint(buf, 10, &value);
	if (err) {
		dev_err(dev, "only decimal value allowed, but(%s)\n", buf);
		return err;
	}

	if (value > MAX_TRANSACTION_SAVA_NR)
		value = MAX_TRANSACTION_SAVA_NR;
	else if (value <= 0)
		value = 0;	/* disable */

	/* clear */
	mutex_lock(&ipcq_stat->trans_mtx);
	list_for_each_safe(work_node, tmp_node,
			&ipcq_stat->trans_head) {
		if (ipcq_stat->trans_nr <= value)
			break;
		trans_item = list_entry(work_node,
			struct iva_ipcq_trans, node);
		list_del(work_node);
		devm_kfree(dev, trans_item);
		ipcq_stat->trans_nr--;
	}
	mutex_unlock(&ipcq_stat->trans_mtx);

	ipcq_stat->trans_nr_max = (uint32_t) value;
	dev_info(dev, "max ipcq trans %d\n", ipcq_stat->trans_nr_max);
	return count;
}

static ssize_t iva_mcu_log_ctrl_show(struct device *dev,
		struct device_attribute *dev_attr, char *buf)
{
	struct iva_dev_data 	*iva = dev_get_drvdata(dev);
	ssize_t 		buf_size = 0;
	char			tmp_buf[256];
	int			tmp_size;

	buf[0] = 0;
	IVA_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
			"mcu_print_delay (%d usecs)\n",
			iva->mcu_print_delay);
exit:
	return buf_size;
}

static ssize_t iva_mcu_log_ctrl(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
#define MAX_TRANSACTION_SAVA_NR		(40)
	struct iva_dev_data 	*iva = dev_get_drvdata(dev);
	int		value;
	int		err;

	err = kstrtoint(buf, 10, &value);
	if (err) {
		dev_err(dev, "only decimal value allowed, but(%s)\n", buf);
		return err;
	}

	if (value <= 0)
		value = 0;

	iva->mcu_print_delay = (uint32_t) value;

	return count;
}

static DEVICE_ATTR(iva_mem_proc, S_IRUGO | S_IWUSR,
			iva_mem_proc_show, NULL);
static DEVICE_ATTR(iva_mem, S_IRUGO | S_IWUSR,
			iva_mem_show, NULL);
static DEVICE_ATTR(ipcq_trans, S_IRUGO | S_IWUSR,
			iva_ipcq_show_trans, iva_ipcq_set_trans);
static DEVICE_ATTR(mcu_log, S_IRUGO | S_IWUSR,
			iva_mcu_log_ctrl_show, iva_mcu_log_ctrl);

static struct attribute *iva_mem_attributes[] = {
	&dev_attr_iva_mem.attr,
	&dev_attr_iva_mem_proc.attr,
	&dev_attr_ipcq_trans.attr,
	&dev_attr_mcu_log.attr,
	NULL,
};

static struct attribute_group iva_attr_group = {
	.attrs = iva_mem_attributes,
};

int32_t iva_sysfs_init(struct device *dev)
{
	return sysfs_create_group(&dev->kobj, &iva_attr_group);
}

void iva_sysfs_deinit(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &iva_attr_group);
}
