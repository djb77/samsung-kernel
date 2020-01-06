/*
 * Copyright (C) 2016 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/netdevice.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_link_device_shmem.h"

static int save_dump_file(struct link_device *ld, struct io_device *iod,
		unsigned long arg, u8 __iomem *dump_base, size_t dump_size)
{
	size_t copied = 0;
	struct sk_buff *skb;
	unsigned int alloc_size = 0xE00;
	int ret;

	if (dump_size == 0 || dump_base == NULL) {
		mif_err("ERR! save_dump_file fail!\n");
		return -EFAULT;
	}

	ret = copy_to_user((void __user *)arg, &dump_size, sizeof(dump_size));
	if (ret) {
		mif_err("ERR! copy_from_user fail!\n");
		return -EFAULT;
	}

	while (copied < dump_size) {
		if (dump_size - copied < alloc_size)
			alloc_size =  dump_size - copied;

		skb = alloc_skb(alloc_size, GFP_ATOMIC);
		if (!skb) {
			skb_queue_purge(&iod->sk_rx_q);
			mif_err("ERR! alloc_skb fail, purged skb_rx_q\n");
			return -ENOMEM;
		}

		memcpy(skb_put(skb, alloc_size), dump_base + copied,
				alloc_size);
		copied += alloc_size;

		/* Record the IO device and the link device into the &skb->cb */
		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = ld;

		skbpriv(skb)->lnk_hdr = false;
		skbpriv(skb)->exynos_ch = iod->id;

		ret = iod->recv_skb_single(iod, ld, skb);
		if (unlikely(ret < 0)) {
			struct modem_ctl *mc = ld->mc;

			mif_err_limited("%s: %s<-%s: %s->recv_skb fail (%d)\n",
				ld->name, iod->name, mc->name, iod->name, ret);
			dev_kfree_skb_any(skb);
			return ret;
		}
	}

	mif_info("Complete! (%zu bytes)\n", copied);

	return 0;
}

int save_vss_dump(struct link_device *ld, struct io_device *iod,
		unsigned long arg)
{
	struct shmem_link_device *shmd = to_shmem_link_device(ld);
	size_t vss_size = shm_get_vss_size();

	if (vss_size == 0 || shmd->vss_base == NULL) {
		mif_err("ERR! save_vss_dump fail!\n");
		return -EFAULT;
	}

	return save_dump_file(ld, iod, arg, shmd->vss_base, vss_size);
}

int save_acpm_dump(struct link_device *ld, struct io_device *iod,
		unsigned long arg)
{
	struct shmem_link_device *shmd = to_shmem_link_device(ld);
	size_t acpm_size = shm_get_acpm_size();

	if (acpm_size == 0 || shmd->acpm_base == NULL) {
		mif_err("ERR! save_acpm_dump fail!\n");
		return -EFAULT;
	}

	return save_dump_file(ld, iod, arg, shmd->acpm_base, acpm_size);
}

int save_shmem_dump(struct link_device *ld, struct io_device *iod,
		unsigned long arg)
{
	struct shmem_link_device *shmd = to_shmem_link_device(ld);
	size_t shmem_size = shmd->size;

	if (shmem_size == 0 || shmd->base == NULL) {
		mif_err("ERR! save_shmem_dump fail!\n");
		return -EFAULT;
	}

	return save_dump_file(ld, iod, arg, (u8 __iomem *)shmd->base, shmem_size);
}

#if 0
void save_mem_dump(struct mem_link_device *mld)
{
#ifdef CONFIG_DEBUG_MODEM_IF
	struct link_device *ld = &mld->link_dev;
	char *path = mld->dump_path;
	struct file *fp;
	struct utc_time t;

	get_utc_time(&t);
	snprintf(path, MIF_MAX_PATH_LEN, "%s/%s_%d%02d%02d_%02d%02d%02d.dump",
		MIF_LOG_DIR, ld->name, t.year, t.mon, t.day, t.hour, t.min,
		t.sec);

	fp = mif_open_file(path);
	if (!fp) {
		mif_err("%s: ERR! %s open fail\n", ld->name, path);
		return;
	}
	mif_err("%s: %s opened\n", ld->name, path);

	mif_save_file(fp, mld->base, mld->size);

	mif_close_file(fp);
#endif
}

void mem_dump_work(struct work_struct *ws)
{
#ifdef CONFIG_DEBUG_MODEM_IF
	struct mem_link_device *mld;

	mld = container_of(ws, struct mem_link_device, dump_work);
	if (!mld) {
		mif_err("ERR! no mld\n");
		return;
	}

	save_mem_dump(mld);
#endif
}
#endif
