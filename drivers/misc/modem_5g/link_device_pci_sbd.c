/*
 * Copyright (C) 2011 Samsung Electronics.
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

#include <net/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include <linux/workqueue.h>

#include <linux/dma-mapping.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"
#include "include/sbd.h"

#ifdef GROUP_MEM_LINK_SBD
/**
@weakgroup group_mem_link_sbd
@{
*/

#ifdef GROUP_MEM_LINK_SETUP
/**
@weakgroup group_mem_link_setup
@{
*/

#ifdef DEBUG_TX_TEST
static enum hrtimer_restart sbd_test_timer_func(struct hrtimer *timer)
{
	struct sbd_link_device *sl =
		container_of(timer, struct sbd_link_device, sbd_test_timer);
	struct sbd_ring_buffer *rb;

	unsigned int i = 0, loop_cnt = 0;

	rb = &sl->ipc_dev[1].rb[UL];

	loop_cnt = (rb->len > 200) ? 200 : rb->len;

	// UL DMA alloc test
	mif_err("+++\n");

	do {
		int ret;
		struct sk_buff *skb;
		char *buff;
		dma_addr_t dma_addr;

		skb = alloc_skb(SBD_DMA_SKB_MAX_SIZE, GFP_DMA);
		if (!skb) {
			mif_err("ERR! alloc_skb fail (size:%d)\n",
					SBD_DMA_SKB_MAX_SIZE);
			return -ENOMEM;
		}

		//skb_reserve(skb, SBD_DMA_SKB_MAX_SIZE);
		buff = skb_put(skb, SBD_DMA_SKB_MAX_SIZE);
		memcpy(buff, "KTG123", 6);

		dma_addr = pci_map_single(sl->pdev, skb->data,
				SBD_DMA_SKB_MAX_SIZE,
				DMA_TO_DEVICE);

		ret = pci_dma_mapping_error(sl->pdev, dma_addr);
		if (ret) {
			mif_err("ERR(%d)! PCI DMA mapping for RX buffers has failed\n", ret);
			kfree_skb(skb);
			return HRTIMER_NORESTART;
		}

		skbpriv(skb)->dma_addr = dma_addr;
		rb->dma_desc[i].ext_hi_addr = HIGH_WORD(dma_addr);
		rb->dma_desc[i].ext_lo_addr = LOW_WORD(dma_addr);
		rb->dma_desc[i].size = skb->len;

		skb_queue_tail(&rb->skb_q, skb);

		rb->dma_buffer[i].va_buffer = skb;

#if 0
		if (i < 2) {
			mif_err("<KTG> [%s] rb->dma_desc[%d]-dma_addr:%x%x (real:%llx)\n",
					(dir == DL) ? "DL" : "UL",
					i,
					rb->dma_desc[i].ext_hi_addr,
					rb->dma_desc[i].ext_lo_addr,
					dma_addr);

			mif_err("dma_desc->size:%d, skb->len:%d\n", rb->dma_desc[i].size, skb->len);
		}
#endif

	} while (i++ < loop_cnt);
	/////////////////////////////

	*rb->wp += 200;
	rb->sbd_intr->wp_intr = 1;

#if 0
	if (*rb->wp < 10) {
		//PKT pass test

		mif_err("<KTG> [UL] rb->dma_desc[%d]-dma_addr:%x%x size:%d\n",
				*rb->wp,
				rb->dma_desc[*rb->wp].ext_hi_addr,
				rb->dma_desc[*rb->wp].ext_lo_addr,
				rb->dma_desc[*rb->wp].size);

		mif_err("<KTG> sbd UL intr(%pK) test(1) - [intr wp:0x%x] [intr rp:0x%x]\n",
			rb->sbd_intr,
			rb->sbd_intr->wp_intr,
			rb->sbd_intr->rp_intr);

		*rb->wp += 1;
		rb->sbd_intr->wp_intr = 1;

		mif_err("<KTG> sbd UL intr(%pK) test(2) - [intr wp:0x%x] [intr rp:0x%x]\n",
			rb->sbd_intr,
			rb->sbd_intr->wp_intr,
			rb->sbd_intr->rp_intr);

		rb->sbd_intr->wp_intr = 0;

		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	}
#endif

	mif_err("rb[UL] in:%d, out:%d\n", *rb->rp, *rb->wp);
	mif_err("---(%d)\n", i);

	return HRTIMER_NORESTART;
}
#endif

static void print_sbd_config(struct sbd_link_device *sl)
{
#ifdef DEBUG_MODEM_IF
	int i;
#ifdef DEBUG_KTG
	u32 *tmp = (u32 *)sl->shmem;
	pr_err("mif: MAGIC : %x\n", *tmp);
#endif

	pr_err("mif: SBD_IPC {shmem_base:0x%lX shmem_size:%d}\n",
		(unsigned long)sl->shmem, sl->shmem_size);

	pr_err("mif: SBD_IPC {version:%d num_channels:%d rbps_offset:%d}\n",
		sl->g_desc->version, sl->g_desc->num_channels,
		sl->g_desc->rbps_offset);

	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_rb_channel *rb_ch = &sl->g_desc->rb_ch[i];
		struct sbd_rb_desc *rbd;

		rbd = &sl->g_desc->rb_desc[i][UL];
		pr_err("mif: RB_DESC[%-2d][UL](offset:%d) = "
			"{id:%-2d ch:%-3d dir:%s} "
			"{sbdv_offset:%-5d rb_len:%-3d} "
			"{buff_size:%-4d payload_offset:%d}\n",
			i, rb_ch->ul_rbd_offset, rbd->id, rbd->ch,
			udl_str(rbd->direction), rb_ch->ul_sbdv_offset,
			rbd->length, rbd->buff_size, rbd->payload_offset);

		rbd = &sl->g_desc->rb_desc[i][DL];
		pr_err("mif: RB_DESC[%-2d][DL](offset:%d) = "
			"{id:%-2d ch:%-3d dir:%s} "
			"{sbdv_offset:%-5d rb_len:%-3d} "
			"{buff_size:%d payload_offset:%d}\n",
			i, rb_ch->dl_rbd_offset, rbd->id, rbd->ch,
			udl_str(rbd->direction), rb_ch->dl_sbdv_offset,
			rbd->length, rbd->buff_size, rbd->payload_offset);
	}

#ifdef DEBUG_TX_TEST
	if (1) {
	ktime_t ktime = ktime_set(0, ms2ns(2000));

	hrtimer_init(&sl->sbd_test_timer,
		CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	sl->sbd_test_timer.function = sbd_test_timer_func;

	hrtimer_start(&sl->sbd_test_timer, ktime, HRTIMER_MODE_REL);
	}
#endif
}

//#ifdef CONFIG_LINK_DEVICE_PCI
#if 1
static void init_intr_alloc(struct sbd_link_device *sl, unsigned int offset)
{
	sl->intr_alloc_offset = offset;
}

static void *intr_alloc(struct sbd_link_device *sl, size_t size)
{
	u8 *intr = sl->shmem_intr + sl->intr_alloc_offset;
	sl->intr_alloc_offset += size;
	return intr;
}
#endif

static void init_desc_alloc(struct sbd_link_device *sl, unsigned int offset)
{
	sl->desc_alloc_offset = offset;
}

static void *desc_alloc(struct sbd_link_device *sl, size_t size)
{
	u8 *desc = (sl->shmem + sl->desc_alloc_offset);
	sl->desc_alloc_offset += size;
	return desc;
}

static void init_buff_alloc(struct sbd_link_device *sl, unsigned int offset)
{
	sl->buff_alloc_offset = offset;
}

static u8 *buff_alloc(struct sbd_link_device *sl, unsigned int size)
{
	u8 *buff = (sl->shmem + sl->buff_alloc_offset);
	sl->buff_alloc_offset += size;
	return buff;
}

/**
@brief		set up an SBD RB descriptor in SHMEM
@{
*/
static void setup_sbd_rb_desc(struct sbd_rb_desc *rb_desc,
			      struct sbd_ring_buffer *rb)
{
	rb_desc->ch = rb->ch;

	rb_desc->direction = rb->dir;
	rb_desc->signaling = 1;

	rb_desc->sig_mask = MASK_INT_VALID | MASK_SEND_DATA;

	rb_desc->length = rb->len;
	rb_desc->id = rb->id;

	rb_desc->buff_size = rb->buff_size;
	rb_desc->payload_offset = rb->payload_offset;
}

static void setup_link_attr(struct sbd_link_attr *link_attr, u16 id, u16 ch,
			    struct modem_io_t *io_dev)
{
	link_attr->id = id;
	link_attr->ch = ch;

	if (io_dev->attrs & IODEV_ATTR(ATTR_NO_LINK_HEADER))
		link_attr->lnk_hdr = false;
	else
		link_attr->lnk_hdr = true;

	link_attr->rb_len[UL] = io_dev->ul_num_buffers;
	link_attr->buff_size[UL] = io_dev->ul_buffer_size;
	link_attr->rb_len[DL] = io_dev->dl_num_buffers;
	link_attr->buff_size[DL] = io_dev->dl_buffer_size;
}

static void setup_desc_rgn(struct sbd_link_device *sl)
{
	u8 rbps_init = 0;
	size_t size;

#ifdef DEBUG_KTG
	mif_err("SHMEM {base:0x%pK size:%d}\n",
		sl->shmem, sl->shmem_size);
#endif

	/*
	Allocate @g_desc.
	*/
	size = sizeof(struct sbd_global_desc);
	sl->g_desc = (struct sbd_global_desc *)desc_alloc(sl, size);

#ifdef DEBUG_KTG
	mif_err("G_DESC_OFFSET = %d(0x%pK)\n",
		calc_offset(sl->g_desc, sl->shmem),
		sl->g_desc);

	mif_err("RB_CH_OFFSET = %d(size:%d) (0x%pK)\n",
		calc_offset(sl->g_desc->rb_ch, sl->shmem),
		calc_offset(sl->g_desc->rb_ch, sl->g_desc),
		sl->g_desc->rb_ch);

	mif_err("RBD_PAIR_OFFSET = %d(size:%d) (0x%pK)\n",
		calc_offset(sl->g_desc->rb_desc, sl->shmem),
		calc_offset(sl->g_desc->rb_desc, sl->g_desc->rb_ch),
		sl->g_desc->rb_desc);
#endif

	size = sizeof(u16) * ULDL * RDWR * sl->num_channels;
	sl->rbps = (u16 *)buff_alloc(sl, size);

	while (rbps_init < (ULDL * RDWR * sl->num_channels))
		*(sl->rbps + rbps_init++) = 0;

	if (!sl->local_rbps)
		sl->local_rbps = kzalloc(size, GFP_KERNEL);
	sl->local_rp[UL] = sl->local_rbps + sl->num_channels * 0;
	sl->local_wp[UL] = sl->local_rbps + sl->num_channels * 1;
	sl->local_rp[DL] = sl->local_rbps + sl->num_channels * 2;
	sl->local_wp[DL] = sl->local_rbps + sl->num_channels * 3;

#ifdef DEBUG_KTG
	mif_err("RBPS_OFFSET = %d(addr:%pK)\n", calc_offset(sl->rbps, sl->shmem), sl->rbps);
#endif

	/*
	Set up @g_desc.
	*/
	sl->g_desc->version = sl->version;
	sl->g_desc->num_channels = sl->num_channels;
	sl->g_desc->rbps_offset = calc_offset(sl->rbps, sl->shmem);

	/*
	Set up pointers to each RBP array.
	*/
	sl->rp[UL] = sl->rbps + sl->num_channels * 0;
	sl->wp[UL] = sl->rbps + sl->num_channels * 1;
	sl->rp[DL] = sl->rbps + sl->num_channels * 2;
	sl->wp[DL] = sl->rbps + sl->num_channels * 3;

	mif_info("Complete!!\n");
}

#if 0
static int setup_sbd_buffer(struct sbd_link_device *sl, struct sbd_ring_buffer *rb,
			enum direction dir)
{
	/*
	(1) Allocate an array of data buffers in SHMEM.
	(2) Register the address of each data buffer.
	*/

	//rb->pbuff_rgn = (u32 *)buff_alloc(sl, SZ_2K);
	/*
	 * Just test code (temporarily)
	 * ch 0 need to allocate 4K each UL, DL side (total 8K)
	 * ch 1 need to allocate 10240b each UL, DL side (total 20480b)
	 */
	if (rb->id == 0)
		rb->pbuff_rgn = (u32 *)buff_alloc(sl, SZ_4K);
	else if (rb->id == 1)
		rb->pbuff_rgn = (u32 *)buff_alloc(sl, 10240);

	if (!rb->pbuff_rgn)
		return -ENOMEM;

#ifdef DEBUG_KTG
	mif_err("RB[%d:%d][%s] buff_rgn {addr:0x%p offset:%d}\n",
		rb->id, rb->ch, udl_str(dir), rb->pbuff_rgn,
		calc_offset(rb->pbuff_rgn, sl->shmem));
#endif

	return 0;
}
#endif

void rx_alloc_skb_work(struct work_struct *work)
{
	struct sbd_link_device *sl =
		container_of(work, struct sbd_link_device, rx_alloc_skb_work);
	int i;

	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, RX);

		pci_dma_alloc(rb);
	}
}

void tx_alloc_skb_work(struct work_struct *work)
{
	struct sbd_link_device *sl =
		container_of(work, struct sbd_link_device, tx_alloc_skb_work);
	struct sbd_ring_buffer *rb = sbd_id2rb(sl, sl->tx_confirm_irq, TX);

	pci_sbd_tx_clear(rb);
}

int pci_dma_alloc(struct sbd_ring_buffer *rb)
{
	struct mem_link_device *mld = to_mem_link_device(rb->ld);
	unsigned int qlen = rb->len;
	u16 local_in, in;
	int ret;

	unsigned long flags;
	u16 rcvd = 0;

	local_in = *rb->local_rp;

	spin_lock_irqsave(&rb->lock, flags);
	in = *rb->rp;
	spin_unlock_irqrestore(&rb->lock, flags);

	if (local_in == in)
		return 0;

	trace_mif_time_chk_with_rb("Realloc start", 0, in, local_in, *rb->wp,  __func__);

	while (local_in != in) {
		dma_addr_t dma_addr;
		struct sk_buff *skb;

		//skb = alloc_skb(SBD_DMA_SKB_MAX_SIZE, GFP_DMA);
		skb = alloc_skb(get_test_tp_size(), GFP_DMA | GFP_KERNEL);
		if (!skb) {
			mif_err("ERR! alloc_skb fail (size:%d)\n",
					//SBD_DMA_SKB_MAX_SIZE);
					get_test_tp_size());
			trace_mif_err("ERR! alloc_skb fail", get_test_tp_size(), __func__);
			return -ENOMEM;
		}

		//skb_reserve(skb, SBD_DMA_SKB_MAX_SIZE);

		dma_addr = dma_map_single(&(mld->pdev->dev), skb->data,
				//SBD_DMA_SKB_MAX_SIZE,
				get_test_tp_size(),
				DMA_FROM_DEVICE);

		ret = dma_mapping_error(&(mld->pdev->dev), dma_addr);
		if (ret) {
			mif_err("ERR(%d)! PCI DMA mapping for RX buffers has failed\n", ret);
			trace_mif_err("DMA mapping failed", 0, __func__);
			kfree_skb(skb);
			return -EINVAL;
		}


		skbpriv(skb)->dma_addr = dma_addr;
		rb->dma_desc[in].ext_hi_addr = HIGH_WORD(dma_addr);
		rb->dma_desc[in].ext_lo_addr = LOW_WORD(dma_addr);

		rb->dma_local_desc[in].ext_hi_addr = HIGH_WORD(dma_addr);
		rb->dma_local_desc[in].ext_lo_addr = LOW_WORD(dma_addr);
		rb->dma_local_desc[in].size = 0;

		skb_queue_tail(&rb->skb_q, skb);

		rb->dma_buffer[in].va_buffer = skb;

		in = circ_new_ptr(qlen, in, 1);

		rcvd += 1;
	}

	spin_lock_irqsave(&rb->lock, flags);
	*rb->rp = in;
	spin_unlock_irqrestore(&rb->lock, flags);

	trace_mif_time_chk_with_rb("Realloc done", rcvd, in, *rb->local_rp, *rb->wp, __func__);

	return 0;
}

static int setup_sbd_dma(struct sbd_link_device *sl, struct sbd_ring_buffer *rb,
			enum direction dir, struct sbd_link_attr *link_attr)
{
	size_t alloc_size;

	alloc_size = (link_attr->rb_len[dir] * sizeof(struct sbd_dma_desc));
	rb->dma_desc = (struct sbd_dma_desc *)buff_alloc(sl, alloc_size);

	if (!rb->dma_local_desc)
		rb->dma_local_desc = kzalloc(alloc_size, GFP_KERNEL);

	if (!rb->dma_buffer) {
		alloc_size = (link_attr->rb_len[dir] * sizeof(struct dma_buffer_match));
		rb->dma_buffer = kzalloc(alloc_size, GFP_KERNEL);
	}

	if ((!rb->dma_desc) || (!rb->dma_buffer) || (!rb->dma_local_desc)) {
		mif_err("ERR! failed to alloc dma_desc\n");
		goto cleanup;
	}

#ifdef DEBUG_KTG
	mif_err("<KTG> rb[%s]->dma_desc base addr:%pK (rb_len:%d)\n",
			(dir == DL) ? "DL" : "UL", rb->dma_desc, link_attr->rb_len[dir]);
#endif

	// RX buffer
	if (dir == DL) {
		u16 i = 0;

		for (i = 0; i < link_attr->rb_len[dir]; i++)
		{
			struct sk_buff *skb;
			dma_addr_t dma_addr;
			int ret;

			//skb = alloc_skb(SBD_DMA_SKB_MAX_SIZE, GFP_DMA);
			skb = alloc_skb(get_test_tp_size(), GFP_DMA | GFP_KERNEL);
			if (!skb) {
				mif_err("ERR! alloc_skb fail (size:%d, i=%d)\n",
					//SBD_DMA_SKB_MAX_SIZE);
					get_test_tp_size(), i);
				return -ENOMEM;
			}

			//skb_reserve(skb, SBD_DMA_SKB_MAX_SIZE);

			dma_addr = dma_map_single(&(sl->pdev->dev), skb->data,
						//SBD_DMA_SKB_MAX_SIZE,
						get_test_tp_size(),
						DMA_FROM_DEVICE);

			ret = dma_mapping_error(&(sl->pdev->dev), dma_addr);
			if (ret) {
				mif_err("ERR(%d)! PCI DMA mapping for RX buffers has failed\n", ret);
				kfree_skb(skb);
				goto cleanup;
			}

			skbpriv(skb)->dma_addr = dma_addr;
			rb->dma_desc[i].ext_hi_addr = HIGH_WORD(dma_addr);
			rb->dma_desc[i].ext_lo_addr = LOW_WORD(dma_addr);
			rb->dma_desc[i].size = 0;

#if 1
			rb->dma_local_desc[i].ext_hi_addr = HIGH_WORD(dma_addr);
			rb->dma_local_desc[i].ext_lo_addr = LOW_WORD(dma_addr);
			rb->dma_local_desc[i].size = 0;
#endif
			skb_queue_tail(&rb->skb_q, skb);

			rb->dma_buffer[i].va_buffer = skb;
/*
			trace_mif_dma_log(0, "RX", rb->id, rb->ch, i,
					  rb->dma_buffer[i].va_buffer, skb,
					  rb->dma_desc[i].size, skb->len,
					  dma_addr,
					  __func__);
*/
#ifdef DEBUG_KTG
			if (i < 2) {
				mif_err("<KTG> [%s] rb->dma_desc[%d]-dma_addr:%x%x (real:%llx) size:%d\n",
					(dir == DL) ? "DL" : "UL",
					i,
					rb->dma_desc[i].ext_hi_addr,
					rb->dma_desc[i].ext_lo_addr,
					dma_addr,
					rb->dma_desc[i].size);
				mif_err("<KTG> [%d] skb:%pK, va_buffer:%pK\n", i, skb, rb->dma_buffer[i].va_buffer);
			}
#endif
		}
	}

	return 0;

cleanup:
	/*
	 * In this case, it should be determined that all of skb will be free
	 * or retry or Do other option.
	 * At now, we don`t care the skbs allocated already and just return
	 * ENODEV
	 */
	return -ENODEV;
}

static int setup_sbd_rb(struct sbd_link_device *sl, struct sbd_ring_buffer *rb,
			enum direction dir, struct sbd_link_attr *link_attr)
{
	rb->sl = sl;

	rb->lnk_hdr = link_attr->lnk_hdr;

	rb->more = false;
	rb->total = 0;
	rb->rcvd = 0;

	/*
	Initialize an SBD RB instance in the kernel space.
	*/
	rb->id = link_attr->id;
	rb->ch = link_attr->ch ?: SIPC_CH_ID_PDP_0;
	rb->dir = dir;
	rb->len = link_attr->rb_len[dir];
	rb->buff_size = link_attr->buff_size[dir];
	rb->payload_offset = 0;

#if 1
	/* For tachyon */
	rb->sbd_intr = (struct sbd_intr *)intr_alloc(sl, sizeof(struct sbd_intr));
	mif_err("(Tachyon) rb[%s] ch:%d, sbd_intr:%p (wp:0x%x)(rp:0x%x)\n",
		(dir == DL) ? "DL" : "UL",
		rb->ch,
		rb->sbd_intr,
		rb->sbd_intr->wp_intr,
		rb->sbd_intr->rp_intr);
#endif

	/*
	Prepare SBD array in SHMEM.
	*/
	rb->rp = &sl->rp[rb->dir][rb->id];
	rb->wp = &sl->wp[rb->dir][rb->id];

	rb->local_rp = &sl->local_rp[rb->dir][rb->id];
	rb->local_wp = &sl->local_wp[rb->dir][rb->id];

#ifdef DEBUG_KTG
	mif_err("rb[%s] ch:%d rb->rp:%pK, rb->wp:%pK, local rp:%pK, local wp:%pK\n",
		(dir == DL) ? "DL" : "UL",
		rb->ch,
		rb->rp,
		rb->wp,
		rb->local_rp,
		rb->local_wp);
#endif

	rb->iod = link_get_iod_with_channel(sl->ld, rb->ch);
	rb->ld = sl->ld;

	return 0;
}

static int init_sbd_ipc(struct sbd_link_device *sl,
			struct sbd_ipc_device ipc_dev[],
			struct sbd_link_attr link_attr[])
{
	struct sbd_ring_buffer *rb;
	int ret;
	int i;

	setup_desc_rgn(sl);

	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_rb_channel *rb_ch = &sl->g_desc->rb_ch[i];
		struct sbd_rb_desc *rb_desc;

		ipc_dev[i].id = link_attr[i].id;
		ipc_dev[i].ch = link_attr[i].ch;

		// UL DMA desc
		rb = &ipc_dev[i].rb[UL];
		ret = setup_sbd_dma(sl, rb, UL, &link_attr[i]);
		if (ret < 0)
			return ret;
		rb_ch->ul_sbdv_offset = calc_offset(rb->dma_desc, sl->shmem);

		//DL DMA desc
		rb = &ipc_dev[i].rb[DL];
		ret = setup_sbd_dma(sl, rb, DL, &link_attr[i]);
		if (ret < 0)
			return ret;
		rb_ch->dl_sbdv_offset = calc_offset(rb->dma_desc, sl->shmem);

		/*
		Setup UL Ring Buffer in the ipc_dev[$i]
		*/
		rb = &ipc_dev[i].rb[UL];
		ret = setup_sbd_rb(sl, rb, UL, &link_attr[i]);
		if (ret < 0)
			return ret;

		/*
		Setup UL RB_DESC & UL RB_CH in the g_desc
		*/
		rb_desc = &sl->g_desc->rb_desc[i][UL];
		setup_sbd_rb_desc(rb_desc, rb);
		rb_ch->ul_rbd_offset = calc_offset(rb_desc, sl->shmem);

		/*
		Setup DL Ring Buffer in the ipc_dev[$i]
		*/
		rb = &ipc_dev[i].rb[DL];
		ret = setup_sbd_rb(sl, rb, DL, &link_attr[i]);
		if (ret < 0)
			return ret;

		/*
		Setup DL RB_DESC & DL RB_CH in the g_desc
		*/
		rb_desc = &sl->g_desc->rb_desc[i][DL];
		setup_sbd_rb_desc(rb_desc, rb);
		rb_ch->dl_rbd_offset = calc_offset(rb_desc, sl->shmem);
	}
/*
	for (i = 0; i < sl->num_channels; i++) {
		rb = &ipc_dev[i].rb[UL];
		ret = setup_sbd_buffer(sl, rb, UL);
		if (ret < 0)
			return ret;

		rb = &ipc_dev[i].rb[DL];
		ret = setup_sbd_buffer(sl, rb, DL);
		if (ret < 0)
			return ret;
	}
*/
	return 0;
}

int init_pci_sbd_intr(struct sbd_link_device *sl,
		  u8 *base, u32 size)
{
	u8 *tmp;
	u32 i;

	if (!sl || !base)
		return -EINVAL;

	sl->shmem_intr = base;
	sl->shmem_intr_size = size;

	tmp = (u8 *)sl->shmem_intr;
	for (i = 0; i < (sizeof(struct sbd_intr) * sl->num_channels); i++)
		tmp[i] = 0;

	return 0;
}

int init_pci_sbd_bar4(struct sbd_link_device *sl,
		  u8 *base, u32 size)
{
	u8 *tmp;
	u32 i;

	if (!sl || !base)
		return -EINVAL;

	sl->shmem_bar4 = base;
	sl->shmem_bar4_size = size;

	tmp = (u8 *)sl->shmem_bar4;
	for (i = 0; i < size; i++)
		tmp[i] = 0;

	return 0;
}
#endif

int init_pci_sbd_link(struct sbd_link_device *sl)
{
	int err;
	u32 *magic;
	u8 *tmp;
	u32 i;

	if (!sl)
		return -ENOMEM;

	tmp = (u8 *)sl->shmem;
	for (i = 0; i < SZ_2K; i++)
		tmp[i + SZ_2K] = 0;

	init_desc_alloc(sl, DESC_RGN_OFFSET);
	init_buff_alloc(sl, CMD_PCI_DESC_RGN_SIZE);
	init_intr_alloc(sl, 0);

	err = init_sbd_ipc(sl, sl->ipc_dev, sl->link_attr);
	if (!err) {
		magic = (u32 *)sl->shmem;
		*magic = 0xab;

		print_sbd_config(sl);
	}
	sbd_activate(sl);

	return err;
}

static void init_ipc_device(struct sbd_link_device *sl, u16 id,
			    struct sbd_ipc_device *ipc_dev)
{
	u16 ch = sbd_id2ch(sl, id);
	struct sbd_ring_buffer *rb;

	ipc_dev->id = id;
	ipc_dev->ch = ch;

	atomic_set(&ipc_dev->config_done, 0);

	rb = &ipc_dev->rb[UL];
	spin_lock_init(&rb->lock);
	skb_queue_head_init(&rb->skb_q);
	mif_err("<KTG> RB skb init RB dir:%d, rb ch:%d, rb id:%d(priv:%pK)\n", UL, ch, id, &rb->skb_q);

	rb = &ipc_dev->rb[DL];
	spin_lock_init(&rb->lock);
	skb_queue_head_init(&rb->skb_q);
	mif_err("<KTG> RB skb init RB dir:%d, rb ch:%d, rb id:%d(priv:%pK)\n", DL, ch, id, &rb->skb_q);
}

/**
@return		the number of actual link channels
*/
static unsigned int init_ctrl_tables(struct sbd_link_device *sl, int num_iodevs,
				     struct modem_io_t iodevs[])
{
	int i;
	unsigned int id;
	unsigned int dummy_idx = -1;

	/*
	Fill ch2id array with MAX_LINK_CHANNELS value to prevent sbd_ch2id()
	from returning 0 for unused channels.
	*/
	for (i = 0; i < MAX_SIPC_CHANNELS; i++)
		sl->ch2id[i] = MAX_LINK_CHANNELS;

	for (id = 0, i = 0; i < num_iodevs; i++) {
		int ch = iodevs[i].id;

		//if (sipc5_ipc_ch(ch) && !sipc_ps_ch(ch)) {
		if (sipc5_ipc_ch(ch)) {
			/* Save CH# to LinkID-to-CH conversion table. */
			sl->id2ch[id] = ch;

			/* Save LinkID to CH-to-LinkID conversion table. */
			sl->ch2id[ch] = id;

			/* Set up the attribute table entry of a LinkID. */
			setup_link_attr(&sl->link_attr[id], id, ch, &iodevs[i]);

			++id;
		} else if (iodevs[i].format == IPC_MULTI_RAW) {
			dummy_idx = i;
		}
	}
#if 0
	for (i = 0; i < num_iodevs; i++) {
		int ch = iodevs[i].id;

		if (sipc_ps_ch(ch)) {
			sl->id2ch[id] = 0;
			sl->ch2id[ch] = id;
		}
	}
#endif
	if ((dummy_idx > 0) && (dummy_idx < num_iodevs)) {
	/* Set up the attribute table entry of a LinkID. */
	setup_link_attr(&sl->link_attr[id],
			id, iodevs[dummy_idx].id, &iodevs[dummy_idx]);

	id++;
	}

	/* Finally, id has the number of actual link channels. */
	return id;
}

u32 pci_bar4_dbgfs_read_index;
u32 pci_bar2_dbgfs_read_index;
u32 pci_bar0_dbgfs_read_index;

static ssize_t dbgfs_bar2_pci_log(struct file *file,
		char __user *user_buf, size_t count, loff_t *ppos)
{
	struct sbd_link_device *sl = file->private_data;
	u32 buf_size;

	if (!sl)
		return 0;

	if (pci_bar2_dbgfs_read_index > SZ_64K) {
		pci_bar2_dbgfs_read_index = 0;
		return 0;
	}

	buf_size = (SZ_2K > count) ? count : SZ_2K;
	memcpy(user_buf, sl->shmem + pci_bar2_dbgfs_read_index, buf_size);
	pci_bar2_dbgfs_read_index += buf_size;

	*ppos += buf_size;

	return buf_size;
}

static ssize_t dbgfs_bar0_pci_log(struct file *file,
		char __user *user_buf, size_t count, loff_t *ppos)
{
	struct sbd_link_device *sl = file->private_data;
	u32 buf_size;

	if (!sl)
		return 0;

	if (pci_bar0_dbgfs_read_index > SZ_2K) {
		pci_bar0_dbgfs_read_index = 0;
		return 0;
	}

	buf_size = (SZ_2K > count) ? count : SZ_2K;
	memcpy(user_buf, sl->shmem_intr + pci_bar0_dbgfs_read_index, buf_size);
	pci_bar0_dbgfs_read_index += buf_size;

	*ppos += buf_size;

	return buf_size;
}

static ssize_t dbgfs_bar4_pci_log(struct file *file,
		char __user *user_buf, size_t count, loff_t *ppos)
{
	struct sbd_link_device *sl = file->private_data;
	u32 buf_size;

	if (!sl)
		return 0;

	if (pci_bar4_dbgfs_read_index > SZ_64K) {
		pci_bar4_dbgfs_read_index = 0;
		return 0;
	}

	mif_info("[MIF] %s called\n", __func__);
	buf_size = (SZ_2K > count) ? count : SZ_2K;
	memcpy(user_buf, sl->shmem_bar4 + pci_bar4_dbgfs_read_index, buf_size);
	pci_bar4_dbgfs_read_index += buf_size;

	*ppos += buf_size;

	mif_info("[MIF] %s called, done\n", __func__);
	return buf_size;
}

static ssize_t dbgfs_ap_force_dump(struct file *file,
		const char __user *data, size_t count, loff_t *ppos)
{
	struct sbd_link_device *sl = file->private_data;
	struct link_device *ld = sl->ld;
	struct io_device *iod = link_get_iod_with_channel(sl->ld,
					SIPC5_CH_ID_BOOT_1);

	if (!ld || !iod)
		return -EINVAL;

	mif_info("[MIF] %s called\n", __func__);
	modem_pci_force_dump(ld, iod);

	return count;
}

static const struct file_operations dbgfs_bar4_frame_fops = {
	.open = simple_open,
	.read = dbgfs_bar4_pci_log,
	.owner = THIS_MODULE
};

static const struct file_operations dbgfs_bar2_frame_fops = {
	.open = simple_open,
	.read = dbgfs_bar2_pci_log,
	.owner = THIS_MODULE
};

static const struct file_operations dbgfs_bar0_frame_fops = {
	.open = simple_open,
	.read = dbgfs_bar0_pci_log,
	.owner = THIS_MODULE
};

static const struct file_operations dbgfs_ap_force_dump_fops = {
	.open = simple_open,
	.write = dbgfs_ap_force_dump,
	.owner = THIS_MODULE
};

int create_pci_sbd_link_device(struct link_device *ld, struct sbd_link_device *sl,
			   u8 *shmem_base, unsigned int shmem_size)
{
	int i;
	int num_iodevs;
	struct modem_io_t *iodevs;
	struct mem_link_device *mld;
	struct dentry *d;

	if (!ld || !sl || !shmem_base)
		return -EINVAL;

	if (!ld->mdm_data)
		return -EINVAL;

	mld = to_mem_link_device(ld);
	sl->pdev = mld->pdev;

	num_iodevs = ld->mdm_data->num_iodevs;
	iodevs = ld->mdm_data->iodevs;

	sl->ld = ld;

	sl->version = 1;

	sl->shmem = shmem_base;
	sl->shmem_size = shmem_size;

	sl->num_channels = init_ctrl_tables(sl, num_iodevs, iodevs);

	mif_err("ld name:%s, num_iodevs:%d, sl->num_ch:%d\n",
		ld->name, num_iodevs, sl->num_channels);

	for (i = 0; i < sl->num_channels; i++)
		init_ipc_device(sl, i, sbd_id2dev(sl, i));

	pci_bar4_dbgfs_read_index = 0;
	pci_bar2_dbgfs_read_index = 0;
	pci_bar0_dbgfs_read_index = 0;
	sl->dbgfs_dir = debugfs_create_dir("svnet", NULL);
	sl->pci_bar4_map_dbgfs = debugfs_create_file("pci_bar4_map_log", S_IRUGO,
				sl->dbgfs_dir, sl, &dbgfs_bar4_frame_fops);
	sl->pci_bar2_map_dbgfs = debugfs_create_file("pci_bar2_map_log", S_IRUGO,
				sl->dbgfs_dir, sl, &dbgfs_bar2_frame_fops);
	sl->pci_bar0_map_dbgfs = debugfs_create_file("pci_bar0_map_log", S_IRUGO,
				sl->dbgfs_dir, sl, &dbgfs_bar0_frame_fops);
	d = debugfs_create_file("ap_force_dump", 0220,
				sl->dbgfs_dir, sl, &dbgfs_ap_force_dump_fops);

	/*
	 * It will be dedicated to RX dma buffer allocation
	 */
	sl->rx_alloc_work_queue = create_singlethread_workqueue("pci_rx_alloc");
	if (NULL == sl->rx_alloc_work_queue)
		mif_err("Failed to create PCI RX Alloc work queue.\n");

	INIT_WORK(&sl->rx_alloc_skb_work, rx_alloc_skb_work);


	/*
	 * It will be dedicated to TX dma buffer allocation
	 */
	sl->tx_alloc_work_queue = create_singlethread_workqueue("pci_tx_alloc");
	if (NULL == sl->tx_alloc_work_queue)
		mif_err("Failed to create PCI TX Alloc work queue.\n");

	INIT_WORK(&sl->tx_alloc_skb_work, tx_alloc_skb_work);

	return 0;
}

/**
@}
*/
#endif

#ifdef GROUP_MEM_IPC_TX
/**
@weakgroup group_mem_ipc_tx
@{
*/

/**
@brief		check the free space in a SBD RB

@param rb	the pointer to an SBD RB instance

@retval "> 0"	the size of free space in the @b @@dev TXQ
@retval "< 0"	an error code
*/
static inline int check_rb_space(struct sbd_ring_buffer *rb, unsigned int qlen,
				 unsigned int in, unsigned int out)
{
	int space;

	if (!circ_valid(qlen, in, out)) {
		mif_err("ERR! TXQ[%d:%d] DIRTY (qlen:%d in:%d out:%d)\n",
			rb->id, rb->ch, qlen, in, out);
		return -EIO;
	}

	space = (in < out) ? (out - in - 1) : (qlen + out - in - 1);

	if (unlikely(space < 0)) {
		mif_err_limited("TXQ[%d:%d] NOSPC (qlen:%d in:%d out:%d)\n",
				rb->id, rb->ch, qlen, in, out);
		return -ENOSPC;
	}

	return space;
}

int pci_sbd_pio_tx(struct sbd_ring_buffer *rb, struct sk_buff *skb)
{
	struct mem_link_device *mld;
	int ret;
	dma_addr_t dma_addr;
	u16 qlen = rb->len;
	u16 out = *rb->local_wp;
	u16 in = *rb->local_rp;

	ret = check_rb_space(rb, qlen, in, out);
	if (unlikely(ret < 0))
		return ret;

	mld = to_mem_link_device(rb->ld);

	dma_addr = dma_map_single(&(mld->pdev->dev), skb->data,
				skb->len, DMA_TO_DEVICE);

	ret = dma_mapping_error(&(mld->pdev->dev), dma_addr);
	if (ret) {
		mif_err("ERR(%d)! DMA mapping failed(rb->ch:%d)\n",
			ret, rb->ch);
		return -ENOSPC;
	}

	skbpriv(skb)->dma_addr = dma_addr;
	rb->dma_desc[out].ext_hi_addr = HIGH_WORD(dma_addr);
	rb->dma_desc[out].ext_lo_addr = LOW_WORD(dma_addr);
	rb->dma_desc[out].size = skb->len;

	rb->dma_buffer[out].va_buffer = skb;

	*rb->local_wp = circ_new_ptr(qlen, out, 1);

	return skb->len;
}

/**
@}
*/
#endif

#ifdef GROUP_MEM_IPC_RX
/**
@weakgroup group_mem_ipc_rx
@{
*/

static inline void set_lnk_hdr(struct sbd_ring_buffer *rb, struct sk_buff *skb)
{
	skbpriv(skb)->lnk_hdr = rb->lnk_hdr && !rb->more;
}

static inline void set_skb_priv(struct sbd_ring_buffer *rb, struct sk_buff *skb)
{
	skbpriv(skb)->iod = rb->iod;
	skbpriv(skb)->ld = rb->ld;
	skbpriv(skb)->sipc_ch = rb->ch;
}

static inline void check_more(struct sbd_ring_buffer *rb, struct sk_buff *skb)
{
	if (rb->lnk_hdr) {
		if (!rb->more) {
			if (sipc5_get_frame_len(skb->data) > rb->buff_size) {
				rb->more = true;
				rb->total = sipc5_get_frame_len(skb->data);
				rb->rcvd = skb->len;
			}
		} else {
			rb->rcvd += skb->len;
			if (rb->rcvd >= rb->total) {
				rb->more = false;
				rb->total = 0;
				rb->rcvd = 0;
			}
		}
	}
}

struct UDP_datagram {
		signed   int id      : 32;
		unsigned int tv_sec  : 32;
		unsigned int tv_usec : 32;
};

static void iperf_test(struct sk_buff *skb)
{
	struct iphdr *ip_header;
	struct udphdr *udp_header;
	static signed int count1 = 0;
	static int count2 = 1;
	struct timeval time_now;

//	u16 tmp;
//	u32 tmp2;
	/*
	struct UDP_datagram {
		signed   int id      : 32;
		unsigned int tv_sec  : 32;
		unsigned int tv_usec : 32;
	} * iperf_header;
	*/
	struct UDP_datagram *iperf_header;
	do_gettimeofday(&time_now);
	skb->ip_summed = CHECKSUM_UNNECESSARY;

	//-------------------------------------------
	ip_header = (struct iphdr *)skb->data;
	udp_header = (struct udphdr *)((char *)(skb->data)+(ip_header->ihl << 2));
	iperf_header = (struct UDP_datagram *)((char *)udp_header + sizeof(struct udphdr));
	//---------------------------------------------------------------
	iperf_header->id = htonl(count1++);
	iperf_header->tv_sec = htonl(time_now.tv_sec);
	iperf_header->tv_usec = htonl(time_now.tv_usec);
	//---------------------------------------------------------------
	ip_header->id = count2++;
	ip_header->check = 0;
	ip_header->check = ip_fast_csum((unsigned char *)ip_header, ip_header->ihl);
}

struct sk_buff *pci_sbd_pio_rx(struct sbd_ring_buffer *rb)
{
	struct mem_link_device *mld = to_mem_link_device(rb->ld);
	u16 out;
	unsigned int qlen = rb->len;
	dma_addr_t dma_addr;
	struct sk_buff *skb;
	u32 dmalen;

	out = *rb->local_rp;

	//trace_mif_time_chk("rx step <1>", out, __func__);

	skb = skb_dequeue(&rb->skb_q);

	if (!skb) {
		mif_err("rb ch:%d[%d] skb is NULL!\n", rb->ch, out);
		trace_mif_err("rb skb is NULL", out, __func__);
		return NULL;
	}

	if (skb != rb->dma_buffer[out].va_buffer) {
		mif_err("ERR! skb is not matched!([%d]-skb:%pK, va_buffer:%pK)",
			out, skb, rb->dma_buffer[out].va_buffer);

		trace_mif_err("ERR! skb is not matched", out, __func__);

		trace_mif_dma_unmatch("RX", rb->id, rb->ch, out,
				      rb->dma_buffer[out].va_buffer, skb,
				      __func__);
		goto exit;
	}

	//trace_mif_time_chk("rx step <2>", out, __func__);

	dmalen = rb->dma_desc[out].size;
	rb->dma_local_desc[out].size = dmalen;

	skb->len = dmalen;
	skb_trim(skb, dmalen);

	//trace_mif_time_chk("rx step <3>", out, __func__);

	if (dmalen > rb->buff_size) {
		mif_err("ERR! RB {id:%d ch%d} size %d > space %d\n",
			rb->id, rb->ch,
			rb->dma_local_desc[out].size, rb->buff_size);

		goto exit;
	}

#if 1
	//for test loopback
	if ((skb->data[1] == 10) || (skb->data[1] == 11))
	{
		skb_pull(skb, 4);

		iperf_test(skb);
#if 0
	} else {
		/*
		mif_err("<KTG> rb[id:%d, ch:%d] rp-%d(local:%d), wp-%d = rx sipc:[0x%x] [0x%x] [0x%x%x] {skb:%pK, vf:%pK}\n",
			rb->id, rb->ch, *rb->rp, *rb->local_rp, *rb->wp,
			skb->data[0], skb->data[1],
			skb->data[2], skb->data[3],
			skb, rb->dma_buffer[out].va_buffer);
		*/
		mif_err("ERR! skb data broken!(lo_rp:%d)\n", out);

		trace_mif_err("ERR! skb data broken", out, __func__);

		goto exit;
#endif
	}

	//trace_mif_time_chk("rx step <4>", out, __func__);
#endif
	//trace_mif_time_chk("rx step <5>", out, __func__);

	dma_addr = skbpriv(skb)->dma_addr;

	dma_unmap_single(&(mld->pdev->dev), dma_addr,
			rb->dma_local_desc[out].size, DMA_FROM_DEVICE);

	rb->dma_buffer[out].va_buffer = NULL;

	*rb->local_rp = circ_new_ptr(qlen, out, 1);

	set_lnk_hdr(rb, skb);
	set_skb_priv(rb, skb);
	check_more(rb, skb);

	//trace_mif_time_chk("rx step <6>", out, __func__);

	return skb;

exit:
	dma_addr = skbpriv(skb)->dma_addr;

	dma_unmap_single(&(mld->pdev->dev), dma_addr,
			rb->dma_local_desc[out].size, DMA_FROM_DEVICE);

	rb->dma_buffer[out].va_buffer = NULL;

	*rb->local_rp = circ_new_ptr(qlen, out, 1);

	if (skb)
		dev_kfree_skb_any(skb);

	return NULL;
}

int pci_sbd_tx_clear(struct sbd_ring_buffer *rb)
{
	struct mem_link_device *mld = to_mem_link_device(rb->ld);
	u16 in, qlen, local_in;
	dma_addr_t dma_addr;
	struct sk_buff *skb;
	unsigned long flags;

	qlen = rb->len;
	local_in = *rb->local_rp;

	spin_lock_irqsave(&rb->lock, flags);
	in = *rb->rp;
	spin_unlock_irqrestore(&rb->lock, flags);

	while (in != local_in) {
		skb = (struct sk_buff *)rb->dma_buffer[local_in].va_buffer;
		if (!skb) {
			mif_err_limited("ERR! skb is NULL in tx_clear! RB {ch:%d} rp:%d(local_rp:%d), wp:%d\n",
				rb->ch, *rb->rp, *rb->local_rp, *rb->wp);

			goto next;
		}

		dma_addr = skbpriv(skb)->dma_addr;

		dma_unmap_single(&(mld->pdev->dev), dma_addr,
				rb->dma_desc[local_in].size, DMA_TO_DEVICE);

		dev_kfree_skb_any(skb);
next:
		rb->dma_buffer[local_in].va_buffer = NULL;

		*rb->local_rp = local_in = circ_new_ptr(qlen, local_in, 1);
	}

	return 0;
}

/**
@}
*/
#endif

/**
// End of group_mem_link_sbd
@}
*/
#endif
