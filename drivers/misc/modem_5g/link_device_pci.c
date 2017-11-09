/*
 * Copyright (C) 2010 Samsung Electronics.
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

#define CREATE_TRACE_POINTS

#include <linux/irq.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>

#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/suspend.h>
#include <linux/pm_qos.h>
#if defined(CONFIG_SOC_EXYNOS3475)
#include <mach/smc.h>
#else
#include <linux/smc.h>
#endif

#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>

#include <linux/shm_ipc.h>
#include <linux/mcu_ipc.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"
#include "link_ctrlmsg_iosm.h"
#include "pmu-cp.h"

extern void exynos_pcie_poweron(int);
extern void exynos_pcie_poweroff(int);

int modem_pcie_init(struct mem_link_device *mld);

static const struct pci_device_id modem_pcie_device_id[] = {
	{ MODEM_PCIE_VENDOR_ID,
	MODEM_PCIE_DEVICE_ID,
	PCI_ANY_ID,
	PCI_ANY_ID,
	0,
	0,
	0,
	},
	{0,},
};

static const struct of_device_id modem_pcie_match[] = {
	{
		.compatible = "sec_modem,modem_pci_pdata",
		.data = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(pci, modem_pcie_device_id);
MODULE_DEVICE_TABLE(of, modem_pcie_match);

struct wake_lock test_wlock;
static int pm_enable = 0;
module_param(pm_enable, int, S_IRUGO);
MODULE_PARM_DESC(pm_enable, "5G(PCI) PM enable");

atomic_t test_napi;

//모든 static 등 함수 block
#if 1

#ifdef GROUP_MEM_LINK_COMMAND

#ifdef DEBUG_TX_TEST
static irqreturn_t modem_pci_irq_handler(int irq, void *data)
{
	struct mem_link_device *mld = (struct mem_link_device *)data;
	struct sbd_link_device *sl;
	struct sbd_ring_buffer *rb;

	u16 in, out;
	u8 *tmp;
	struct sbd_ring_buffer *ul_rb;

	mif_err("+++\n");

	if (!mld)
		goto error;

	sl = &mld->sbd_link_dev;
	if (!sl)
		goto error;

	rb = &sl->ipc_dev[1].rb[DL];
	if (!rb)
		goto error;

	/*
	 DL intr register clear
	 */
	tmp = (u8 *)sl->shmem_intr;
	*(tmp + PCI_BAR0_DL0_IRQ) = 0;

	in = *rb->rp;
	out = *rb->wp;

	mif_err("rb[DL] in:%d, out:%d\n", in, out);

	ul_rb = &sl->ipc_dev[1].rb[UL];

	mif_err("(orig) rb[UL] in:%d, out:%d\n", *ul_rb->rp, *ul_rb->wp);
	while (in != out) {
		struct sk_buff *skb, *ul_skb;

		skb = skb_dequeue(&rb->skb_q);
		ul_skb = skb_dequeue(&ul_rb->skb_q);

		if (skb != rb->dma_buffer[in].va_buffer) {
			mif_err("skb is not matched!([%d]-skb:%p, va_buffer:%p)\n",
				out, skb, rb->dma_buffer[in].va_buffer);
		}

		in += 1;
	}

	*rb->rp = in;
	mif_err("---\n");

	return IRQ_HANDLED;
error:
	mif_err("xxx\n");

	return IRQ_HANDLED;
}
#else
static irqreturn_t modem_pci_irq_handler(int irq, void *data)
{
	struct mem_link_device *mld = (struct mem_link_device *)data;
	struct link_device *ld;
	struct mst_buff *msb;
	struct sbd_link_device *sl;
	volatile u64 value1, value2;
	int i;

	if (!mld)
		return IRQ_HANDLED;

#ifdef CONFIG_LINK_DEVICE_NAPI
	if (!atomic_read(&test_napi))
		return IRQ_HANDLED;
#endif

	mif_debug("+++\n");

	ld = &mld->link_dev;

	/* bar0's type was changed to u32 from u8
	 * since type u8 can't access all interrupt register.
	 */
	value1 = *(mld->bar0 + PCI_OB_DB_PENDING_0);
	value2 = *(mld->bar0 + PCI_OB_DB_PENDING_1);
	mld->multi_irq = value1 + (value2 << 32);
	mif_info("OB PEND %llx\n", mld->multi_irq);

	if(!mld->multi_irq) {
		mif_info("no IRQ!!\n");
		return IRQ_HANDLED;
	}

	if (mld->multi_irq & PCI_IRQ_TX_DONE) {
		sl = &mld->sbd_link_dev;
		for (i = 0; i < sl->num_channels; i++) {
			if (((mld->multi_irq >> (i * 2)) & 0x01) == 0x01) {
				mif_debug("tx_confirm i = [%d]\n", i);
				sl->tx_confirm_irq = i;
				queue_work(sl->tx_alloc_work_queue, &sl->tx_alloc_skb_work);
			}
		}
	}
	if (mld->multi_irq & PCI_IRQ_RX) {
		msb = mem_take_snapshot(mld, RX);
		if (!msb)
			return IRQ_HANDLED;
#if 0
	if (unlikely(!int_valid(msb->snapshot.int2ap))) {
		mif_err("%s: ERR! invalid intr 0x%X\n",
				ld->name, msb->snapshot.int2ap);
		msb_free(msb);
		return;
	}

	if (unlikely(!rx_possible(mc))) {
		mif_err("%s: ERR! %s.state == %s\n", ld->name, mc->name,
			mc_state(mc));
		msb_free(msb);
		return;
	}
#endif

		msb_queue_tail(&mld->msb_rxq, msb);
		//tasklet_schedule(&mld->rx_tsk);
		queue_delayed_work(ld->rx_wq, &mld->udl_rx_dwork, 0);
	}

#if 1
	/*
	 DL intr register clear
	 */
	sl = &mld->sbd_link_dev;
	if (!sl) {
//		mif_err("xxx\n");
		return IRQ_HANDLED;
	}

	// *((u32 *)(sl->shmem_intr) + PCI_BAR0_DL0_IRQ) = 0x00;
#endif
	return IRQ_HANDLED;
}
#endif

static inline void reset_ipc_map(struct mem_link_device *mld)
{
	int i;

	for (i = 0; i < MAX_SIPC5_DEVICES; i++) {
		struct mem_ipc_device *dev = mld->dev[i];

		set_txq_head(dev, 0);
		set_txq_tail(dev, 0);
		set_rxq_head(dev, 0);
		set_rxq_tail(dev, 0);
	}
}

static int modem_pci_reset_ipc_link(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	unsigned int magic;
	unsigned int access;
	int i;

	set_access(mld, 0);
	set_magic(mld, 0);

	reset_ipc_map(mld);

	for (i = 0; i < MAX_SIPC5_DEVICES; i++) {
		struct mem_ipc_device *dev = mld->dev[i];

		skb_queue_purge(dev->skb_txq);
		atomic_set(&dev->txq.busy, 0);
		dev->req_ack_cnt[TX] = 0;

		skb_queue_purge(dev->skb_rxq);
		atomic_set(&dev->rxq.busy, 0);
		dev->req_ack_cnt[RX] = 0;
	}

	atomic_set(&ld->netif_stopped, 0);

	set_magic(mld, MEM_IPC_MAGIC);
	set_access(mld, 1);

	magic = get_magic(mld);
	access = get_access(mld);
	if (magic != MEM_IPC_MAGIC || access != 1)
		return -EACCES;

	return 0;
}

#endif

#ifdef GROUP_MEM_LINK_DEVICE

static inline bool ipc_active(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	return true;

	if (unlikely(!cp_online(mc))) {
		mif_err("%s<->%s: %s.state %s != ONLINE <%pf>\n",
			ld->name, mc->name, mc->name, mc_state(mc), CALLER);
		return false;
	}

	if (mld->dpram_magic) {
		unsigned int magic = get_magic(mld);
		unsigned int access = get_access(mld);
		if (magic != MEM_IPC_MAGIC || access != 1) {
			mif_err("%s<->%s: ERR! magic:0x%X access:%d <%pf>\n",
				ld->name, mc->name, magic, access, CALLER);
			return false;
		}
	}

	if (atomic_read(&mld->forced_cp_crash)) {
		mif_err("%s<->%s: ERR! forced_cp_crash:%d <%pf>\n",
			ld->name, mc->name, atomic_read(&mld->forced_cp_crash),
			CALLER);
		return false;
	}

	return true;
}

static inline void purge_txq(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	int i;

	/* Purge the skb_txq in every IPC device (IPC_FMT, IPC_RAW, etc.) */
	if (ld->sbd_ipc) {
		struct sbd_link_device *sl = &mld->sbd_link_dev;

		for (i = 0; i < sl->num_channels; i++) {
			struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, TX);
			skb_queue_purge(&rb->skb_q);
		}
	}

	for (i = 0; i < MAX_SIPC5_DEVICES; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		skb_queue_purge(dev->skb_txq);
	}
}

#endif

#ifdef GROUP_MEM_CP_CRASH

static void set_modem_state(struct mem_link_device *mld, enum modem_state state)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;
	struct io_device *iod;

	spin_lock_irqsave(&mc->lock, flags);
	list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
		if (iod && atomic_read(&iod->opened) > 0)
			iod->modem_state_changed(iod, state);
	}
	spin_unlock_irqrestore(&mc->lock, flags);
}

static void modem_pci_handle_cp_crash(struct mem_link_device *mld,
		enum modem_state state)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (mld->stop_pm)
		mld->stop_pm(mld);
#endif

	/* Disable normal IPC */
	set_magic(mld, MEM_CRASH_MAGIC);
	set_access(mld, 0);

	stop_net_ifaces(ld);
	purge_txq(mld);

	if (cp_online(mc) || cp_booting(mc))
		set_modem_state(mld, state);

	atomic_set(&mld->forced_cp_crash, 0);
}

static void handle_no_cp_crash_ack(unsigned long arg)
{
	struct mem_link_device *mld = (struct mem_link_device *)arg;
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	if (cp_crashed(mc)) {
		mif_debug("%s: STATE_CRASH_EXIT without CRASH_ACK\n",
			ld->name);
	} else {
		mif_err("%s: ERR! No CRASH_ACK from CP\n", ld->name);
		modem_pci_handle_cp_crash(mld, STATE_CRASH_EXIT);
	}
}

static void modem_pci_forced_cp_crash(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	/* Disable normal IPC */
	set_magic(mld, MEM_CRASH_MAGIC);
	set_access(mld, 0);

	if (atomic_inc_return(&mld->forced_cp_crash) > 1) {
		mif_err("%s: ALREADY in progress <%pf>\n",
			ld->name, CALLER);
		return;
	}

	if (!cp_online(mc) && !cp_booting(mc)) {
		mif_err("%s: %s.state %s != ONLINE <%pf>\n",
			ld->name, mc->name, mc_state(mc), CALLER);
		return;
	}

	/* Disable debug Snapshot */
	mif_set_snapshot(false);

	if (mld->attrs & LINK_ATTR(LINK_ATTR_MEM_DUMP)) {
		stop_net_ifaces(ld);

		if (mld->debug_info)
			mld->debug_info();

		/**
		 * If there is no CRASH_ACK from CP in a timeout,
		 * handle_no_cp_crash_ack() will be executed.
		 */
		mif_add_timer(&mld->crash_ack_timer, FORCE_CRASH_ACK_TIMEOUT,
			      handle_no_cp_crash_ack, (unsigned long)mld);

		/* Send CRASH_EXIT command to a CP */
		send_ipc_irq(mld, cmd2int(CMD_CRASH_EXIT));
	} else {
		modem_pci_forced_cp_crash(mld);
	}

	mif_err("%s->%s: CP_CRASH_REQ <%pf>\n", ld->name, mc->name, CALLER);
}

#endif

static bool rild_ready(struct link_device *ld)
{
	struct io_device *fmt_iod;
	struct io_device *rfs_iod;
	int fmt_opened;
	int rfs_opened;

	fmt_iod = link_get_iod_with_channel(ld, SIPC5_CH_ID_FMT_0);
	if (!fmt_iod) {
		mif_err("%s: No FMT io_device\n", ld->name);
		return false;
	}

	rfs_iod = link_get_iod_with_channel(ld, SIPC5_CH_ID_RFS_0);
	if (!rfs_iod) {
		mif_err("%s: No RFS io_device\n", ld->name);
		return false;
	}

	fmt_opened = atomic_read(&fmt_iod->opened);
	rfs_opened = atomic_read(&rfs_iod->opened);
	mif_err("%s: %s.opened=%d, %s.opened=%d\n", ld->name,
		fmt_iod->name, fmt_opened, rfs_iod->name, rfs_opened);
	if (fmt_opened > 0 && rfs_opened > 0)
		return true;

	return false;
}

static void cmd_init_start_handler(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	int err;

	mif_err("%s: INIT_START <- %s (%s.state:%s cp_boot_done:%d)\n",
		ld->name, mc->name, mc->name, mc_state(mc),
		atomic_read(&mld->cp_boot_done));

	if (!ld->sbd_ipc) {
		mif_err("%s: LINK_ATTR_SBD_IPC is NOT set\n", ld->name);
		return;
	}

	err = init_sbd_link(&mld->sbd_link_dev);
	if (err < 0) {
		mif_err("%s: init_sbd_link fail(%d)\n", ld->name, err);
		return;
	}

	if (mld->attrs & LINK_ATTR(LINK_ATTR_IPC_ALIGNED))
		ld->aligned = true;
	else
		ld->aligned = false;

	sbd_activate(&mld->sbd_link_dev);
	send_ipc_irq(mld, cmd2int(CMD_PIF_INIT_DONE));

	mif_err("%s: PIF_INIT_DONE -> %s\n", ld->name, mc->name);
}

static void cmd_phone_start_handler(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;
	int err;

	mif_err("%s: CP_START <- %s (%s.state:%s cp_boot_done:%d)\n",
		ld->name, mc->name, mc->name, mc_state(mc),
		atomic_read(&mld->cp_boot_done));

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (mld->start_pm)
		mld->start_pm(mld);
#endif

	spin_lock_irqsave(&mld->state_lock, flags);

	if (mld->state == LINK_STATE_IPC) {
		/*
		If there is no INIT_END command from AP, CP sends a CP_START
		command to AP periodically until it receives INIT_END from AP
		even though it has already been in ONLINE state.
		*/
		if (rild_ready(ld)) {
			mif_err("%s: INIT_END -> %s\n", ld->name, mc->name);
			send_ipc_irq(mld, cmd2int(CMD_INIT_END));
		}
		goto exit;
	}

	err = modem_pci_reset_ipc_link(mld);
	if (err) {
		mif_err("%s: modem_pci_reset_ipc_link fail(%d)\n", ld->name, err);
		goto exit;
	}

	if (rild_ready(ld)) {
		mif_err("%s: INIT_END -> %s\n", ld->name, mc->name);
		send_ipc_irq(mld, cmd2int(CMD_INIT_END));
		atomic_set(&mld->cp_boot_done, 1);
	}

	mld->state = LINK_STATE_IPC;
	complete_all(&mc->init_cmpl);

exit:
	spin_unlock_irqrestore(&mld->state_lock, flags);
}

static void cmd_crash_reset_handler(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	spin_lock_irqsave(&mld->state_lock, flags);
	mld->state = LINK_STATE_OFFLINE;
	spin_unlock_irqrestore(&mld->state_lock, flags);

	mif_err("%s<-%s: ERR! CP_CRASH_RESET\n", ld->name, mc->name);

	modem_pci_handle_cp_crash(mld, STATE_CRASH_RESET);
}

static void cmd_crash_exit_handler(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	/* Disable debug Snapshot */
	mif_set_snapshot(false);

	spin_lock_irqsave(&mld->state_lock, flags);
	mld->state = LINK_STATE_CP_CRASH;
	spin_unlock_irqrestore(&mld->state_lock, flags);

	if (timer_pending(&mld->crash_ack_timer))
		del_timer(&mld->crash_ack_timer);

	if (atomic_read(&mld->forced_cp_crash))
		mif_err("%s<-%s: CP_CRASH_ACK\n", ld->name, mc->name);
	else
		mif_err("%s<-%s: ERR! CP_CRASH_EXIT\n", ld->name, mc->name);

	modem_pci_handle_cp_crash(mld, STATE_CRASH_EXIT);
}

static void modem_pci_cmd_handler(struct mem_link_device *mld, u16 cmd)
{
	struct link_device *ld = &mld->link_dev;

	switch (cmd) {
	case CMD_INIT_START:
		cmd_init_start_handler(mld);
		break;

	case CMD_PHONE_START:
		cmd_phone_start_handler(mld);
		break;

	case CMD_CRASH_RESET:
		cmd_crash_reset_handler(mld);
		break;

	case CMD_CRASH_EXIT:
		cmd_crash_exit_handler(mld);
		break;

	default:
		mif_err("%s: Unknown command 0x%04X\n", ld->name, cmd);
		break;
	}
}

#ifdef GROUP_MEM_IPC_TX

static inline int check_txq_space(struct mem_link_device *mld,
				  struct mem_ipc_device *dev,
				  unsigned int qsize, unsigned int in,
				  unsigned int out, unsigned int count)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned int space;

	if (!circ_valid(qsize, in, out)) {
		mif_err("%s: ERR! Invalid %s_TXQ{qsize:%d in:%d out:%d}\n",
			ld->name, dev->name, qsize, in, out);
		return -EIO;
	}

	space = circ_get_space(qsize, in, out);
	if (unlikely(space < count)) {
		if (cp_online(mc)) {
			mif_err("%s: NOSPC %s_TX{qsize:%d in:%d out:%d space:%d len:%d}\n",
				ld->name, dev->name, qsize,
				in, out, space, count);
		}
		return -ENOSPC;
	}

	return space;
}

static int txq_write(struct mem_link_device *mld, struct mem_ipc_device *dev,
		     struct sk_buff *skb)
{
	char *src = skb->data;
	unsigned int count = skb->len;
	char *dst = get_txq_buff(dev);
	unsigned int qsize = get_txq_buff_size(dev);
	unsigned int in = get_txq_head(dev);
	unsigned int out = get_txq_tail(dev);
	int space;

	space = check_txq_space(mld, dev, qsize, in, out, count);
	if (unlikely(space < 0))
		return space;

	barrier();

	circ_write(dst, src, qsize, in, count);

	barrier();

	set_txq_head(dev, circ_new_ptr(qsize, in, count));

	/* Commit the item before incrementing the head */
	smp_mb();

	return count;
}

static int tx_frames_to_dev(struct mem_link_device *mld,
			    struct mem_ipc_device *dev)
{
	struct sk_buff_head *skb_txq = dev->skb_txq;
	int tx_bytes = 0;
	int ret = 0;

	while (1) {
		struct sk_buff *skb;

		skb = skb_dequeue(skb_txq);
		if (unlikely(!skb))
			break;

		ret = txq_write(mld, dev, skb);
		if (unlikely(ret < 0)) {
			/* Take the skb back to the skb_txq */
			skb_queue_head(skb_txq, skb);
			break;
		}
		pktlog_tx_bottom_skb(mld, skb);

		tx_bytes += ret;

#ifdef DEBUG_MODEM_IF_LINK_TX
		mif_pkt(skbpriv(skb)->sipc_ch, "LNK-TX", skb);
#endif

		dev_kfree_skb_any(skb);
	}

	return (ret < 0) ? ret : tx_bytes;
}

static enum hrtimer_restart tx_timer_func(struct hrtimer *timer)
{
	struct mem_link_device *mld;
	struct link_device *ld;
	struct modem_ctl *mc;
	int i;
	bool need_schedule;
	u16 mask;
	unsigned long flags;

	mld = container_of(timer, struct mem_link_device, tx_timer);
	ld = &mld->link_dev;
	mc = ld->mc;

	need_schedule = false;
	mask = 0;

	spin_lock_irqsave(&mc->lock, flags);

	if (unlikely(!ipc_active(mld)))
		goto exit;

#ifdef CONFIG_LINK_POWER_MANAGEMENT_WITH_FSM
	if (mld->link_active) {
		if (!mld->link_active(mld)) {
			need_schedule = true;
			goto exit;
		}
	}
#endif

	for (i = 0; i < MAX_SIPC5_DEVICES; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		int ret;

		if (unlikely(under_tx_flow_ctrl(mld, dev))) {
			ret = check_tx_flow_ctrl(mld, dev);
			if (ret < 0) {
				if (ret == -EBUSY || ret == -ETIME) {
					need_schedule = true;
					continue;
				} else {
					modem_pci_forced_cp_crash(mld);
					need_schedule = false;
					goto exit;
				}
			}
		}

		ret = tx_frames_to_dev(mld, dev);
		if (unlikely(ret < 0)) {
			if (ret == -EBUSY || ret == -ENOSPC) {
				need_schedule = true;
				txq_stop(mld, dev);
				/* If txq has 2 or more packet and 2nd packet
				  has -ENOSPC return, It request irq to consume
				  the TX ring-buffer from CP */
				mask |= msg_mask(dev);
				continue;
			} else {
				modem_pci_forced_cp_crash(mld);
				need_schedule = false;
				goto exit;
			}
		}

		if (ret > 0)
			mask |= msg_mask(dev);

		if (!skb_queue_empty(dev->skb_txq))
			need_schedule = true;
	}

	if (!need_schedule) {
		for (i = 0; i < MAX_SIPC5_DEVICES; i++) {
			if (!txq_empty(mld->dev[i])) {
				need_schedule = true;
				break;
			}
		}
	}

	if (mask)
		send_ipc_irq(mld, mask2int(mask));

exit:
	if (need_schedule) {
		ktime_t ktime = ktime_set(0, ms2ns(TX_PERIOD_MS));
		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	}

	spin_unlock_irqrestore(&mc->lock, flags);

	return HRTIMER_NORESTART;
}

static inline void start_tx_timer(struct mem_link_device *mld,
				  struct hrtimer *timer)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	spin_lock_irqsave(&mc->lock, flags);

	if (unlikely(cp_offline(mc)))
		goto exit;

	if (!hrtimer_is_queued(timer)) {
		ktime_t ktime = ktime_set(0, ms2ns(TX_PERIOD_MS));
		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	}

exit:
	spin_unlock_irqrestore(&mc->lock, flags);
}

static inline void cancel_tx_timer(struct mem_link_device *mld,
				   struct hrtimer *timer)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	spin_lock_irqsave(&mc->lock, flags);

	if (hrtimer_active(timer))
		hrtimer_cancel(timer);

	spin_unlock_irqrestore(&mc->lock, flags);
}

static int tx_frames_to_rb(struct sbd_ring_buffer *rb)
{
	struct sk_buff_head *skb_txq = &rb->skb_q;
	int tx_bytes = 0;
	int ret = 0;

	while (1) {
		struct sk_buff *skb;

		skb = skb_dequeue(skb_txq);
		if (unlikely(!skb))
			break;

		ret = pci_sbd_pio_tx(rb, skb);
		if (unlikely(ret < 0)) {
			/* Take the skb back to the skb_txq */
			skb_queue_head(skb_txq, skb);
			break;
		}

		tx_bytes += ret;
#ifdef DEBUG_MODEM_IF_LINK_TX
		mif_pkt(rb->ch, "LNK-TX", skb);
#endif
	}

	/*
	 * if it is working well, replace to function call later.
	 */
	if (tx_bytes > 0)
		*rb->wp = *rb->local_wp;

	return (ret < 0) ? ret : tx_bytes;
}

#if 1

static enum hrtimer_restart sbd_tx_timer_func(struct hrtimer *timer)
{
	struct mem_link_device *mld =
		container_of(timer, struct mem_link_device, sbd_tx_timer);
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	struct sbd_link_device *sl = &mld->sbd_link_dev;
	int i;
	bool need_schedule = false;
	u16 mask = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&mc->lock, flags);
	if (unlikely(!ipc_active(mld))) {
		spin_unlock_irqrestore(&mc->lock, flags);
		goto exit;
	}
	spin_unlock_irqrestore(&mc->lock, flags);

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (mld->link_active) {
		if (!mld->link_active(mld)) {
			need_schedule = true;
			goto exit;
		}
	}
#endif

	trace_tracing_mark_write("B", current->pid, FUNC);

	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, TX);
		int ret;

		/*
		 * before processing TX, Fanwu recommend to clear tx buffer.
		 */
		//pci_sbd_tx_clear(rb);

		ret = tx_frames_to_rb(rb);

		if (unlikely(ret < 0)) {
			if (ret == -EBUSY || ret == -ENOSPC) {
				need_schedule = true;
				mask = MASK_SEND_DATA;
				continue;
			} else {
				//modem_pci_forced_cp_crash(mld);
				need_schedule = false;
				goto exit;
			}
		}

		if (ret > 0) {
			/* send irq whenever ap send data */
			*((u32 *)(sl->shmem_intr) + (i * 2)) = INTR_TRIGGER;
			mif_debug("send irq: %x\n", i);
			mask = MASK_SEND_DATA;
		}

		if (!skb_queue_empty(&rb->skb_q))
			need_schedule = true;
	}

#if 1
	if (!need_schedule) {
		for (i = 0; i < sl->num_channels; i++) {
			struct sbd_ring_buffer *rb;

			rb = sbd_id2rb(sl, i, TX);
			if (!rb_empty(rb)) {
				need_schedule = true;
				break;
			}
		}
	}
#endif

#if 0
	/* tx_alloc_skb_work is called when AP get confirm irq from cp */
	if (mask) {
		*((u32 *)(sl->shmem_intr) + 3) = INTR_TRIGGER;

		queue_work(sl->tx_alloc_work_queue, &sl->tx_alloc_skb_work);
	}
#endif

	trace_tracing_mark_write("E", current->pid, FUNC);

#if 0
	if (mask) {
		spin_lock_irqsave(&mc->lock, flags);
		if (unlikely(!ipc_active(mld))) {
			spin_unlock_irqrestore(&mc->lock, flags);
			need_schedule = false;
			goto exit;
		}
		spin_unlock_irqrestore(&mc->lock, flags);
	}
#endif

exit:
	if (need_schedule) {
		ktime_t ktime = ktime_set(0, ms2ns(TX_PERIOD_MS));
		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	}

	return HRTIMER_NORESTART;
}
#endif

static int xmit_ipc_to_rb(struct mem_link_device *mld, enum sipc_ch_id ch,
			  struct sk_buff *skb)
{
	int ret;
	struct link_device *ld = &mld->link_dev;
	struct io_device *iod = skbpriv(skb)->iod;
	struct modem_ctl *mc = ld->mc;
	struct sbd_ring_buffer *rb = sbd_ch2rb(&mld->sbd_link_dev, ch, TX);
	struct sk_buff_head *skb_txq;
	unsigned long flags;

	if (!rb) {
		mif_err("%s: %s->%s: ERR! NO SBD RB {ch:%d}\n",
			ld->name, iod->name, mc->name, ch);
		return -ENODEV;
	}

	skb_txq = &rb->skb_q;

//	mif_err("TX RB{id:%d, ch:%d}\n", rb->id, rb->ch);

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->forbid_cp_sleep)
		mld->forbid_cp_sleep(mld);
#endif

	spin_lock_irqsave(&rb->lock, flags);

	if (unlikely(skb_txq->qlen >= rb->len)) {
		mif_err_limited("%s: %s->%s: ERR! {ch:%d} "
				"skb_txq.len %d >= limit %d\n",
				ld->name, iod->name, mc->name, ch,
				skb_txq->qlen, rb->len);
		ret = -EBUSY;
	} else {
		skb->len = min_t(int, skb->len, rb->buff_size);

		ret = skb->len;
		skb_queue_tail(skb_txq, skb);

		start_tx_timer(mld, &mld->sbd_tx_timer);
	}

	spin_unlock_irqrestore(&rb->lock, flags);

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->permit_cp_sleep)
		mld->permit_cp_sleep(mld);
#endif

	return ret;
}
#endif

static int xmit_ipc_to_dev(struct mem_link_device *mld, enum sipc_ch_id ch,
			   struct sk_buff *skb)
{
	int ret;
	struct link_device *ld = &mld->link_dev;
	struct io_device *iod = skbpriv(skb)->iod;
	struct modem_ctl *mc = ld->mc;
	struct mem_ipc_device *dev = mld->dev[dev_id(ch)];
	struct sk_buff_head *skb_txq;

	if (!dev) {
		mif_err("%s: %s->%s: ERR! NO IPC DEV {ch:%d}\n",
			ld->name, iod->name, mc->name, ch);
		return -ENODEV;
	}

	skb_txq = dev->skb_txq;

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->forbid_cp_sleep)
		mld->forbid_cp_sleep(mld);
#endif
	if (unlikely(skb_txq->qlen >= MAX_SKB_TXQ_DEPTH)) {
		mif_err_limited("%s: %s->%s: ERR! %s TXQ.qlen %d >= limit %d\n",
				ld->name, iod->name, mc->name, dev->name,
				skb_txq->qlen, MAX_SKB_TXQ_DEPTH);
		ret = -EBUSY;
	} else {
		ret = skb->len;
		skb_queue_tail(dev->skb_txq, skb);
		start_tx_timer(mld, &mld->tx_timer);
	}

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->permit_cp_sleep)
		mld->permit_cp_sleep(mld);
#endif

	return ret;
}

static int xmit_ipc(struct mem_link_device *mld, struct io_device *iod,
		    enum sipc_ch_id ch, struct sk_buff *skb)
{
	struct link_device *ld = &mld->link_dev;

	if (unlikely(!ipc_active(mld)))
		return -EIO;

	if (ld->sbd_ipc && iod->sbd_ipc) {
		if (likely(sbd_active(&mld->sbd_link_dev)))
			return xmit_ipc_to_rb(mld, ch, skb);
		else
			return -ENODEV;
	} else {
		return xmit_ipc_to_dev(mld, ch, skb);
	}
}

static inline int check_udl_space(struct mem_link_device *mld,
				  struct mem_ipc_device *dev,
				  unsigned int qsize, unsigned int in,
				  unsigned int out, unsigned int count)
{
	struct link_device *ld = &mld->link_dev;
	unsigned int space;

	if (!circ_valid(qsize, in, out)) {
		mif_err("%s: ERR! Invalid %s_TXQ{qsize:%d in:%d out:%d}\n",
			ld->name, dev->name, qsize, in, out);
		return -EIO;
	}

	space = circ_get_space(qsize, in, out);
	if (unlikely(space < count)) {
		mif_err("%s: NOSPC %s_TX{qsize:%d in:%d out:%d free:%d len:%d}\n",
			ld->name, dev->name, qsize, in, out, space, count);
		return -ENOSPC;
	}

	return 0;
}

static inline int udl_write(struct mem_link_device *mld,
			    struct mem_ipc_device *dev, struct sk_buff *skb)
{
	unsigned int count = skb->len;
	char *src = skb->data;
	char *dst = get_txq_buff(dev);
	unsigned int qsize = get_txq_buff_size(dev);
	unsigned int in = get_txq_head(dev);
	unsigned int out = get_txq_tail(dev);
	int space;

	space = check_udl_space(mld, dev, qsize, in, out, count);
	if (unlikely(space < 0))
		return space;

	barrier();

	circ_write(dst, src, qsize, in, count);

	barrier();

	set_txq_head(dev, circ_new_ptr(qsize, in, count));

	/* Commit the item before incrementing the head */
	smp_mb();

	return count;
}

static int xmit_udl(struct mem_link_device *mld, struct io_device *iod,
		    enum sipc_ch_id ch, struct sk_buff *skb)
{
	int ret;
	struct mem_ipc_device *dev = mld->dev[IPC_RAW];
	int count = skb->len;
	int tried = 0;

	while (1) {
		ret = udl_write(mld, dev, skb);
		if (ret == count)
			break;

		if (ret != -ENOSPC)
			goto exit;

		tried++;
		if (tried >= 20)
			goto exit;

		if (in_interrupt())
			mdelay(50);
		else
			msleep(50);
	}

#ifdef DEBUG_MODEM_IF_LINK_TX
	mif_pkt(ch, "LNK-TX", skb);
#endif

	dev_kfree_skb_any(skb);

exit:
	return ret;
}

/*============================================================================*/

#ifdef GROUP_MEM_IPC_RX

static void pass_skb_to_demux(struct mem_link_device *mld, struct sk_buff *skb)
{
	struct link_device *ld = &mld->link_dev;
	struct io_device *iod = skbpriv(skb)->iod;
	int ret;
	u8 ch = skbpriv(skb)->sipc_ch;

	if (unlikely(!iod)) {
		mif_err("%s: ERR! No IOD for CH.%d\n", ld->name, ch);
		dev_kfree_skb_any(skb);
		modem_pci_forced_cp_crash(mld);
		return;
	}

#ifdef DEBUG_MODEM_IF_LINK_RX
	mif_pkt(ch, "LNK-RX", skb);
#endif

	ret = iod->recv_skb_single(iod, ld, skb);
	if (unlikely(ret < 0)) {
		struct modem_ctl *mc = ld->mc;
		mif_err_limited("%s: %s<-%s: ERR! %s->recv_skb fail (%d)\n",
				ld->name, iod->name, mc->name, iod->name, ret);
		dev_kfree_skb_any(skb);
	}
}

static inline void link_to_demux(struct mem_link_device  *mld)
{
	int i;

	for (i = 0; i < MAX_SIPC5_DEVICES; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		struct sk_buff_head *skb_rxq = dev->skb_rxq;

		while (1) {
			struct sk_buff *skb;

			skb = skb_dequeue(skb_rxq);
			if (!skb)
				break;

			pass_skb_to_demux(mld, skb);
		}
	}
}

static void link_to_demux_work(struct work_struct *ws)
{
	struct link_device *ld;
	struct mem_link_device *mld;

	ld = container_of(ws, struct link_device, rx_delayed_work.work);
	mld = to_mem_link_device(ld);

	link_to_demux(mld);
}

static inline void schedule_link_to_demux(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct delayed_work *dwork = &ld->rx_delayed_work;

	/*queue_delayed_work(ld->rx_wq, dwork, 0);*/
	queue_work_on(7, ld->rx_wq, &dwork->work);
}

static struct sk_buff *rxq_read(struct mem_link_device *mld,
				struct mem_ipc_device *dev,
				unsigned int in)
{
	struct link_device *ld = &mld->link_dev;
	struct sk_buff *skb;
	char *src = get_rxq_buff(dev);
	unsigned int qsize = get_rxq_buff_size(dev);
	unsigned int out = get_rxq_tail(dev);
	unsigned int rest = circ_get_usage(qsize, in, out);
	unsigned int len;
	char hdr[SIPC5_MIN_HEADER_SIZE];

	/* Copy the header in a frame to the header buffer */
	circ_read(hdr, src, qsize, out, SIPC5_MIN_HEADER_SIZE);

	/* Check the config field in the header */
	if (unlikely(!sipc5_start_valid(hdr))) {
		mif_err("%s: ERR! %s BAD CFG 0x%02X (in:%d out:%d rest:%d)\n",
			ld->name, dev->name, hdr[SIPC5_CONFIG_OFFSET],
			in, out, rest);
		goto bad_msg;
	}

	/* Verify the length of the frame (data + padding) */
	len = sipc5_get_total_len(hdr);
	if (unlikely(len > rest)) {
		mif_err("%s: ERR! %s BAD LEN %d > rest %d\n",
			ld->name, dev->name, len, rest);
		goto bad_msg;
	}

	/* Allocate an skb */
	skb = mem_alloc_skb(len);
	if (!skb) {
		mif_err("%s: ERR! %s mem_alloc_skb(%d) fail\n",
			ld->name, dev->name, len);
		goto no_mem;
	}

	/* Read the frame from the RXQ */
	circ_read(skb_put(skb, len), src, qsize, out, len);

	/* Update tail (out) pointer to the frame to be read in the future */
	set_rxq_tail(dev, circ_new_ptr(qsize, out, len));

	/* Finish reading data before incrementing tail */
	smp_mb();

#ifdef DEBUG_MODEM_IF
	/* Record the time-stamp */
	getnstimeofday(&skbpriv(skb)->ts);
#endif

	return skb;

bad_msg:
	mif_err("%s%s%s: ERR! BAD MSG: %02x %02x %02x %02x\n",
		ld->name, arrow(RX), ld->mc->name,
		hdr[0], hdr[1], hdr[2], hdr[3]);
	set_rxq_tail(dev, in);	/* Reset tail (out) pointer */
	modem_pci_forced_cp_crash(mld);

no_mem:
	return NULL;
}

static int rx_frames_from_dev(struct mem_link_device *mld,
			      struct mem_ipc_device *dev)
{
	struct link_device *ld = &mld->link_dev;
	unsigned int qsize = get_rxq_buff_size(dev);
	unsigned int in = get_rxq_head(dev);
	unsigned int out = get_rxq_tail(dev);
	unsigned int size = circ_get_usage(qsize, in, out);
	int rcvd = 0;

	if (unlikely(circ_empty(in, out)))
		return 0;

	while (rcvd < size) {
		struct sk_buff *skb;
		u8 ch;
		struct io_device *iod;

		skb = rxq_read(mld, dev, in);
		if (!skb)
			return -ENOMEM;

		pktlog_rx_bottom_skb(mld, skb);

		ch = sipc5_get_ch(skb->data);
		iod = link_get_iod_with_channel(ld, ch);
		if (!iod) {
			mif_err("%s: ERR! [%s]No IOD for CH.%d(out:%u)\n",
				ld->name, dev->name, ch, get_rxq_tail(dev));
			pr_skb("CRASH", skb);
			dev_kfree_skb_any(skb);
			modem_pci_forced_cp_crash(mld);
			break;
		}

		/* Record the IO device and the link device into the &skb->cb */
		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = ld;

		skbpriv(skb)->lnk_hdr = iod->link_header;
		skbpriv(skb)->sipc_ch = ch;

		/* The $rcvd must be accumulated here, because $skb can be freed
		   in pass_skb_to_demux(). */
		rcvd += skb->len;
		pass_skb_to_demux(mld, skb);
	}

	if (rcvd < size) {
		struct link_device *ld = &mld->link_dev;
		mif_err("%s: WARN! rcvd %d < size %d\n", ld->name, rcvd, size);
	}

	return rcvd;
}

static int recv_ipc_frames(struct mem_link_device *mld,
			    struct mem_snapshot *mst)
{
	int i;
	u16 intr = mst->int2ap;

	for (i = 0; i < MAX_SIPC5_DEVICES; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		int rcvd;

#if 0
		print_dev_snapshot(mld, mst, dev);
#endif

		if (req_ack_valid(dev, intr))
			recv_req_ack(mld, dev, mst);

		rcvd = rx_frames_from_dev(mld, dev);
		if (rcvd < 0)
			return rcvd;

		if (req_ack_valid(dev, intr))
			send_res_ack(mld, dev);

		if (res_ack_valid(dev, intr))
			recv_res_ack(mld, dev, mst);
	}
	return 0;
}

static void pass_skb_to_net(struct mem_link_device *mld, struct sk_buff *skb)
{
	struct link_device *ld = &mld->link_dev;
	struct skbuff_private *priv;
	struct io_device *iod;
	int ret;

	priv = skbpriv(skb);
	if (unlikely(!priv)) {
		mif_err("%s: ERR! No PRIV in skb@%p\n", ld->name, skb);
		dev_kfree_skb_any(skb);
		modem_pci_forced_cp_crash(mld);
		return;
	}

	iod = priv->iod;
	if (unlikely(!iod)) {
		mif_err("%s: ERR! No IOD in skb@%p\n", ld->name, skb);
		dev_kfree_skb_any(skb);
		modem_pci_forced_cp_crash(mld);
		return;
	}

#ifdef DEBUG_MODEM_IF_LINK_RX
	mif_pkt(iod->id, "LNK-RX", skb);
#endif

	ret = iod->recv_net_skb(iod, ld, skb);
	if (unlikely(ret < 0)) {
		struct modem_ctl *mc = ld->mc;
		mif_err_limited("%s: %s<-%s: ERR! %s->recv_net_skb fail (%d)\n",
				ld->name, iod->name, mc->name, iod->name, ret);
		dev_kfree_skb_any(skb);
	}
}

static int rx_net_frames_from_rb(struct sbd_ring_buffer *rb, int budget)
{
	int rcvd = 0;
	struct link_device *ld = rb->ld;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	unsigned int num_frames;

	struct sbd_link_device *sl = &mld->sbd_link_dev;

#ifdef CONFIG_LINK_DEVICE_NAPI
	num_frames = min_t(unsigned int,
			   circ_get_usage(rb->len, *rb->wp, *rb->local_rp),
			   budget);

	mif_err("Budget(%d) rb_usage(%d)\n",
		budget,
		circ_get_usage(rb->len, *rb->wp, *rb->local_rp));
#else
	unsigned long flags;

	spin_lock_irqsave(&rb->lock, flags);
	num_frames = circ_get_usage(rb->len, *rb->wp, *rb->local_rp);
	spin_unlock_irqrestore(&rb->lock, flags);
#endif

	while (num_frames--) {
		struct sk_buff *skb;

		skb = pci_sbd_pio_rx(rb);
		if (!skb) {
			mif_info("WARN! sbd_rx skb is NULL\n");

			continue;
		}

		/* The $rcvd must be accumulated here, because $skb can be freed
		   in pass_skb_to_net(). */
		rcvd++;

		pass_skb_to_net(mld, skb);
	}

	//trace_mif_time_chk("[Rx End]", rcvd, __func__);

	queue_work(sl->rx_alloc_work_queue, &sl->rx_alloc_skb_work);

	trace_mif_time_chk_with_rb("RX done", rcvd, *rb->rp, *rb->local_rp, *rb->wp, __func__);
/*
	if (rcvd < num_frames) {
		struct io_device *iod = rb->iod;
		struct link_device *ld = rb->ld;
		struct modem_ctl *mc = ld->mc;
		mif_err("%s: %s<-%s: WARN! rcvd %d < num_frames %d\n",
			ld->name, iod->name, mc->name, rcvd, num_frames);
	}
*/
#ifdef CONFIG_LINK_DEVICE_NAPI
	mif_debug("Rcvd packets:%d\n", rcvd);
#endif
	return rcvd;
}

static int rx_ipc_frames_from_rb(struct sbd_ring_buffer *rb)
{
	int rcvd = 0;
	struct link_device *ld = rb->ld;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	unsigned int num_frames;

	struct sbd_link_device *sl = &mld->sbd_link_dev;
	unsigned long flags;

	spin_lock_irqsave(&rb->lock, flags);
	num_frames = circ_get_usage(rb->len, *rb->wp, *rb->local_rp);
	spin_unlock_irqrestore(&rb->lock, flags);

	trace_mif_time_chk("recv ipc frame", rb->ch, __func__);

	while (num_frames--) {
		struct sk_buff *skb;

		skb = pci_sbd_pio_rx(rb);
		if (!skb) {
			mif_info("WARN! sbd_rx skb is NULL\n");

			continue;
		}

		/* The $rcvd must be accumulated here, because $skb can be freed
		   in pass_skb_to_demux(). */
		rcvd++;

		if (skbpriv(skb)->lnk_hdr) {
			u8 ch = rb->ch;
			u8 fch = sipc5_get_ch(skb->data);
			if (fch != ch) {
				mif_err("frm.ch:%d != rb.ch:%d\n", fch, ch);
				pr_skb("CRASH", skb);
				dev_kfree_skb_any(skb);
				/*
				 * there is no mechanism to crash, yet.
				 */
				//modem_pci_forced_cp_crash(mld);
				continue;
			}
		}

		pass_skb_to_demux(mld, skb);
	}

	queue_work(sl->rx_alloc_work_queue, &sl->rx_alloc_skb_work);
/*
	if (rcvd < num_frames) {
		struct io_device *iod = rb->iod;
		struct modem_ctl *mc = ld->mc;
		mif_err("%s: %s<-%s: WARN! rcvd %d < num_frames %d\n",
			ld->name, iod->name, mc->name, rcvd, num_frames);
	}
*/
	return rcvd;
}

int pci_netdev_poll(struct napi_struct *napi, int budget)
{
	int rcvd;
	struct vnet *vnet = netdev_priv(napi->dev);
	struct link_device *ld = get_current_link(vnet->iod);
	struct mem_link_device *mld =
		container_of(ld, struct mem_link_device, link_dev);
	struct sbd_ring_buffer *rb =
		sbd_ch2rb(&mld->sbd_link_dev, vnet->iod->id, RX);

	rcvd = rx_net_frames_from_rb(rb, budget);

	/* no more ring buffer to process */
	if (rcvd < budget) {
		napi_complete(napi);
		/* To do: enable mailbox irq */
		schedule_work(&mld->polling_timer);
	}

//	mif_debug("%d pkts\n", rcvd);

	return rcvd;
}

#ifdef CONFIG_LINK_DEVICE_NAPI
static void polling_timer_func(struct work_struct *work)
{
	struct mem_link_device *mld =
		container_of(work, struct mem_link_device, polling_timer);
	struct sbd_link_device *sl = &mld->sbd_link_dev;
	int cycle_cnt = 0;
	int max_cycle = 50;
	int i;

//	mif_err("Polling timer start!\n");
	trace_mif_time_chk("polling timer start!", 0, __func__);
	do {
		cycle_cnt++;
		for(i = 0; i < sl->num_channels; i++) {
			struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, RX);
			struct io_device *iod
				= link_get_iod_with_channel(rb->ld, rb->ch);

			if(unlikely(rb_empty(rb)) || rb == NULL)
				continue;

			mif_err("RX RB{id:%d, ch:%d}\n", rb->id, rb->ch);

			if(likely(sipc_ps_ch(rb->ch))) {
				if (napi_schedule_prep(&iod->napi)) {
					mif_err("%s cnt:%d usage:%d \n",
							iod->name,cycle_cnt,rb_usage(rb));
					__napi_schedule(&iod->napi);
					cycle_cnt = 0;
				}
			} else {
				rx_ipc_frames_from_rb(rb);
				cycle_cnt = 0;
			}
		}
		msleep(1);
	} while (cycle_cnt <= max_cycle);

//	mif_err("Polling timer end!\n");
	trace_mif_time_chk("polling timer end!", 0, __func__);
	//mbox_enable_irq(mld->irq_cp2ap_msg);
	atomic_set(&test_napi, 1);
}
#endif

static int recv_sbd_ipc_frames(struct mem_link_device *mld,
				struct mem_snapshot *mst)
{
	struct sbd_link_device *sl = &mld->sbd_link_dev;
	int i;

	trace_tracing_mark_write("B", current->pid, FUNC);

	mif_debug("multi_irq: %llx\n", mld->multi_irq);
	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, RX);
		int rcvd = 0;

		if (((mld->multi_irq >> (i * 2 + 1)) & 0x01) == 0x00)
			continue;
		if (unlikely(rb_empty(rb)))
			continue;

		if (likely(sipc_ps_ch(rb->ch))) {
#ifdef CONFIG_LINK_DEVICE_NAPI
#if 0
			/* To do: disable mailbox irq */
			if (napi_schedule_prep(&rb->iod->napi))
				__napi_schedule(&rb->iod->napi);
#endif
			struct io_device *iod =
				link_get_iod_with_channel(rb->ld, rb->ch);

			if (napi_schedule_prep(&iod->napi)) {
				//mbox_disable_irq(mld->irq_cp2ap_msg);
				atomic_set(&test_napi, 0);
				__napi_schedule(&iod->napi);
			}
#else
			rcvd = rx_net_frames_from_rb(rb, 0);
#endif
		} else {
			rcvd = rx_ipc_frames_from_rb(rb);
		}

		/*
		 * In this case, other RB`s data couldn`t be processed,
		 * before receiving next irq.
		 * It was a legacy code so it needs to be discussed with IPC member.
		 * And now, it will be ignored temporarily.
		 */
//		if (rcvd < 0)
//			return rcvd;

		/* send confirm irq */
		*((u32 *)(sl->shmem_intr) + (i * 2 + 1)) = INTR_TRIGGER;
	}

	trace_tracing_mark_write("E", current->pid, FUNC);

	return 0;
}

static void modem_pci_oom_handler_work(struct work_struct *ws)
{
	struct mem_link_device *mld =
		container_of(ws, struct mem_link_device, page_reclaim_work);
	struct sk_buff *skb;

	/* try to page reclaim with GFP_KERNEL */
	skb = alloc_skb(PAGE_SIZE - 512, GFP_KERNEL);
	if (skb)
		dev_kfree_skb_any(skb);

	/* need to disable the RX irq ?? */
	msleep(200);

	mif_info("trigger the rx task again\n");
	tasklet_schedule(&mld->rx_tsk);
}

static void ipc_rx_func(struct mem_link_device *mld)
{
	struct sbd_link_device *sl = &mld->sbd_link_dev;
	u32 qlen = mld->msb_rxq.qlen;

	//tx complete process
#if 0
	struct sbd_ring_buffer *rb;
	int i;
	for (i = 0; i < sl->num_channels; i++) {
		rb = sbd_id2rb(sl, i, TX);
		pci_sbd_tx_clear(rb);
	}
#endif

	while (qlen-- > 0) {
		struct mst_buff *msb;
		int ret = 0;
//		u16 intr;

		msb = msb_dequeue(&mld->msb_rxq);
		if (!msb)
			break;
/*
		intr = msb->snapshot.int2ap;

		if (cmd_valid(intr))
			mld->cmd_handler(mld, int2cmd(intr));
*/
		if (sbd_active(sl))
			ret = recv_sbd_ipc_frames(mld, &msb->snapshot);
		else
			ret = recv_ipc_frames(mld, &msb->snapshot);

		if (ret == -ENOMEM) {
			msb_queue_head(&mld->msb_rxq, msb);
			mif_err_limited("Rx ENOMEM, enqueue head again");
			/*
			if (!work_pending(&mld->page_reclaim_work)) {
				struct link_device *ld = &mld->link_dev;
				mif_err_limited("Rx ENOMEM, try reclaim work");
				queue_work(ld->rx_wq,
						&mld->page_reclaim_work);
			}
			*/
			return;
		}

		msb_free(msb);
	}

#if 0
	if (mld->msb_rxq.qlen)
		tasklet_schedule(&mld->rx_tsk);
#elif 1
	if (mld->msb_rxq.qlen) {
		struct link_device *ld = &mld->link_dev;

		//mif_err("sorry, re-workqueue!(%d)\n", mld->msb_rxq.qlen);
		queue_delayed_work(ld->rx_wq, &mld->udl_rx_dwork, 0);
	}
#endif
}

static void udl_rx_work(struct work_struct *ws)
{
	struct mem_link_device *mld;

	mld = container_of(ws, struct mem_link_device, udl_rx_dwork.work);

	ipc_rx_func(mld);
}

#if 1
static void modem_pci_rx_task(unsigned long data)
{
	struct mem_link_device *mld = (struct mem_link_device *)data;
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	if (likely(cp_online(mc)))
		ipc_rx_func(mld);
	else
		queue_delayed_work(ld->rx_wq, &mld->udl_rx_dwork, 0);
}
#endif
#endif

/*============================================================================*/

#ifdef GROUP_MEM_LINK_METHOD

static int modem_pci_init_comm(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct modem_ctl *mc = ld->mc;
	struct io_device *check_iod;
	int id = iod->id;
	int fmt2rfs = (SIPC5_CH_ID_RFS_0 - SIPC5_CH_ID_FMT_0);
	int rfs2fmt = (SIPC5_CH_ID_FMT_0 - SIPC5_CH_ID_RFS_0);

	if (atomic_read(&mld->cp_boot_done))
		return 0;

#ifdef CONFIG_LINK_CONTROL_MSG_IOSM
	if (mld->iosm) {
		struct sbd_link_device *sl = &mld->sbd_link_dev;
		struct sbd_ipc_device *sid = sbd_ch2dev(sl, iod->id);

		if (atomic_read(&sid->config_done)) {
			tx_iosm_message(mld, IOSM_A2C_OPEN_CH, (u32 *)&id);
			return 0;
		} else {
			mif_err("%s isn't configured channel\n", iod->name);
			return -ENODEV;
		}
	}
#endif

	switch (id) {
	case SIPC5_CH_ID_FMT_0 ... SIPC5_CH_ID_FMT_9:
		check_iod = link_get_iod_with_channel(ld, (id + fmt2rfs));
		if (check_iod ? atomic_read(&check_iod->opened) : true) {
			mif_err("%s: %s->INIT_END->%s\n",
				ld->name, iod->name, mc->name);
			send_ipc_irq(mld, cmd2int(CMD_INIT_END));
			atomic_set(&mld->cp_boot_done, 1);
		} else {
			mif_err("%s is not opened yet\n", check_iod->name);
		}
		break;

	case SIPC5_CH_ID_RFS_0 ... SIPC5_CH_ID_RFS_9:
		check_iod = link_get_iod_with_channel(ld, (id + rfs2fmt));
		if (check_iod) {
			if (atomic_read(&check_iod->opened)) {
				mif_err("%s: %s->INIT_END->%s\n",
					ld->name, iod->name, mc->name);
				send_ipc_irq(mld, cmd2int(CMD_INIT_END));
				atomic_set(&mld->cp_boot_done, 1);
			} else {
				mif_err("%s not opened yet\n", check_iod->name);
			}
		}
		break;

	default:
		break;
	}

	return 0;
}

static void modem_pci_terminate_comm(struct link_device *ld, struct io_device *iod)
{
#ifdef CONFIG_LINK_CONTROL_MSG_IOSM
	struct mem_link_device *mld = to_mem_link_device(ld);

	if (mld->iosm)
		tx_iosm_message(mld, IOSM_A2C_CLOSE_CH, (u32 *)&iod->id);
#endif
}

static int modem_pci_init_descriptor(struct link_device *ld)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	int ret;

	ret = init_pci_sbd_link(&mld->sbd_link_dev);
	if (ret < 0) {
		mif_err("%s: init_pci_sbd_link fail(%d)\n", ld->name, ret);
		return ret;
	}

	ret = init_pci_sbd_bar4(&mld->sbd_link_dev, mld->boot_base, mld->boot_size);
	if (ret < 0) {
		mif_err("%s: init_pci_sbd_bar4 fail(%d)\n", ld->name, ret);
		return ret;
	}

	return 0;
}

static int modem_pci_send(struct link_device *ld, struct io_device *iod,
		    struct sk_buff *skb)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct modem_ctl *mc = ld->mc;
	enum dev_format id = iod->format;
	u8 ch = iod->id;

	/*
	 * For iperf DL test, send function should be disable.
	 * It is just workaround code. Next time, it will be removed.
	 */

	switch (id) {
	case IPC_RAW:
		if (unlikely(atomic_read(&ld->netif_stopped) > 0)) {
			if (in_interrupt()) {
				mif_err("raw tx is suspended, drop size=%d\n",
						skb->len);
				return -EBUSY;
			}

			mif_err("wait TX RESUME CMD...\n");
			init_completion(&ld->raw_tx_resumed);
			wait_for_completion(&ld->raw_tx_resumed);
			mif_err("TX resumed done.\n");
		}

	case IPC_RFS:
	case IPC_FMT:
		if (likely(sipc5_ipc_ch(ch)))
			return xmit_ipc(mld, iod, ch, skb);
		else
			return xmit_udl(mld, iod, ch, skb);

	case IPC_BOOT:
	case IPC_DUMP:
		if (sipc5_udl_ch(ch))
			return xmit_udl(mld, iod, ch, skb);
		break;

	default:
		break;
	}

	mif_err("%s:%s->%s: ERR! Invalid IO device (format:%s id:%d)\n",
		ld->name, iod->name, mc->name, dev_str(id), ch);

	return -ENODEV;
}

static void modem_pci_boot_on(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	unsigned long flags;

	atomic_set(&mld->cp_boot_done, 0);

	spin_lock_irqsave(&mld->state_lock, flags);
	mld->state = LINK_STATE_OFFLINE;
	spin_unlock_irqrestore(&mld->state_lock, flags);

	cancel_tx_timer(mld, &mld->tx_timer);

	if (ld->sbd_ipc) {
#ifdef CONFIG_LTE_MODEM_XMM7260
		sbd_deactivate(&mld->sbd_link_dev);
#endif
		cancel_tx_timer(mld, &mld->sbd_tx_timer);

		if (mld->iosm) {
			memset(mld->base + CMD_RGN_OFFSET, 0, CMD_RGN_SIZE);
			mif_info("Control message region has been initialized\n");
		}
	}

	purge_txq(mld);
}

#define SS5G_BOOT_INFO		0x0
#define SS5G_BOOT_COMMAND	0x1
#define SS5G_BOOT_DATA		0x2

static int modem_pci_xmit_boot(struct link_device *ld, struct io_device *iod,
		     unsigned long arg)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct ss5g_boot_info boot_info;
	struct ss5g_boot_command boot_command;
	struct ss5g_boot_data boot_data;
	int err;
	int type;
	u8 *bar2_base = mld->base - mld->bar2_offset;
	u8 *bar4_base = mld->boot_base;

	memset(&boot_info, 0, sizeof(struct ss5g_boot_info));
	memset(&boot_command, 0, sizeof(struct ss5g_boot_command));
	memset(&boot_data, 0, sizeof(struct ss5g_boot_data));

	err = copy_from_user(&type, (const void __user *)arg, sizeof(int));
	if (err) {
		mif_err("%s: ERR! INFO copy_from_user fail\n", ld->name);
		return -EFAULT;
	}

	switch (type) {
	case SS5G_BOOT_INFO:
		err = copy_from_user(&boot_info, (const void __user *)arg, sizeof(boot_info));
		if (err) {
			mif_err("%s: ERR! INFO copy_from_user fail\n", ld->name);
			return -EFAULT;
		}
		mif_info("BOOT_INFO: addr 0x%X, b_offset 0x%X, m_offset 0x%X, size 0x%X, crc 0x%X\n",
				boot_info.addr, boot_info.b_offset, boot_info.m_offset,
				boot_info.b_size, boot_info.crc);
		memcpy(bar2_base + boot_info.addr - 0xC000000, &(boot_info.b_offset), sizeof(u32));
		memcpy(bar2_base + boot_info.addr - 0xC000000 + 0x4, &(boot_info.m_offset), sizeof(u32) * 3);
		break;
	case SS5G_BOOT_COMMAND:
		err = copy_from_user(&boot_command, (const void __user *)arg, sizeof(boot_command));
		if (err) {
			mif_err("%s: ERR! COMMAND copy_from_user fail\n", ld->name);
			return -EFAULT;
		}
		mif_info("BOOT_COMMAND: addr 0x%X, value 0x%X\n", boot_command.addr, boot_command.value);
		memcpy(bar2_base + boot_command.addr - 0xC000000, &(boot_command.value), sizeof(u32));
		break;
	case SS5G_BOOT_DATA:
		err = copy_from_user(&boot_data, (const void __user *)arg, sizeof(boot_data));
		if (err) {
			mif_err("%s: ERR! DATA copy_from_user fail\n", ld->name);
			return -EFAULT;
		}
		memcpy(bar4_base, boot_data.binary, boot_data.len);
		mif_info("BOOT_DATA: copied size 0x%x\n", boot_data.len);
		break;
	default:
		mif_info("invalid type: %d\n", type);
		break;
	}


	return 0;
}

static int modem_pci_security_request(struct link_device *ld, struct io_device *iod,
				unsigned long arg)
{
	unsigned long param2, param3;
	int err = 0;
	struct modem_sec_req msr;

	err = copy_from_user(&msr, (const void __user *)arg, sizeof(msr));
	if (err) {
		mif_err("%s: ERR! copy_from_user fail\n", ld->name);
		err = -EFAULT;
		goto exit;
	}

	param2 = shm_get_security_param2(msr.mode, msr.size_boot);
	param3 = shm_get_security_param3(msr.mode, msr.size_main);
	mif_err("mode=%u, size=%lu, addr=%lu\n", msr.mode, param2, param3);
	/* exynos_smc(SMC_ID_CLK, SSS_CLK_ENABLE, 0, 0); */
	err = exynos_smc(SMC_ID, msr.mode, param2, param3);
	/* exynos_smc(SMC_ID_CLK, SSS_CLK_DISABLE, 0, 0); */
	mif_info("%s: return_value=%d\n", ld->name, err);
exit:
	return err;
}

static int modem_pci_start_download(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);

	if (ld->sbd_ipc && mld->attrs & LINK_ATTR(LINK_ATTR_MEM_DUMP))
		sbd_deactivate(&mld->sbd_link_dev);

	reset_ipc_map(mld);

	if (mld->attrs & LINK_ATTR(LINK_ATTR_BOOT_ALIGNED))
		ld->aligned = true;
	else
		ld->aligned = false;

	if (mld->dpram_magic) {
		unsigned int magic;

		set_magic(mld, MEM_BOOT_MAGIC);
		magic = get_magic(mld);
		if (magic != MEM_BOOT_MAGIC) {
			mif_err("%s: ERR! magic 0x%08X != BOOT_MAGIC 0x%08X\n",
				ld->name, magic, MEM_BOOT_MAGIC);
			return -EFAULT;
		}
		mif_err("%s: magic == 0x%08X\n", ld->name, magic);
	}

	return 0;
}

static int modem_pci_update_firm_info(struct link_device *ld, struct io_device *iod,
				unsigned long arg)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	int ret;

	ret = copy_from_user(&mld->img_info, (void __user *)arg,
			     sizeof(struct std_dload_info));
	if (ret) {
		mif_err("ERR! copy_from_user fail!\n");
		return -EFAULT;
	}

	return 0;
}

static int modem_pci_force_dump(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	mif_err("+++\n");
	modem_pci_forced_cp_crash(mld);
	mif_err("---\n");
	return 0;
}

static int modem_pci_start_upload(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);

	if (ld->sbd_ipc && mld->attrs & LINK_ATTR(LINK_ATTR_MEM_DUMP))
		sbd_deactivate(&mld->sbd_link_dev);

	reset_ipc_map(mld);

	if (mld->attrs & LINK_ATTR(LINK_ATTR_DUMP_ALIGNED))
		ld->aligned = true;
	else
		ld->aligned = false;

	if (mld->dpram_magic) {
		unsigned int magic;

		set_magic(mld, MEM_DUMP_MAGIC);
		magic = get_magic(mld);
		if (magic != MEM_DUMP_MAGIC) {
			mif_err("%s: ERR! magic 0x%08X != DUMP_MAGIC 0x%08X\n",
				ld->name, magic, MEM_DUMP_MAGIC);
			return -EFAULT;
		}
		mif_err("%s: magic == 0x%08X\n", ld->name, magic);
	}

	return 0;
}

static int modem_pci_full_dump(struct link_device *ld, struct io_device *iod,
		unsigned long arg)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	unsigned long copied = 0;
	struct sk_buff *skb;
	unsigned int alloc_size = 0xE00;
	int ret;

	ret = copy_to_user((void __user *)arg, &mld->size, sizeof(mld->size));
	if (ret) {
		mif_err("ERR! copy_from_user fail!\n");
		return -EFAULT;
	}

	while (copied < mld->size) {
		if (mld->size - copied < alloc_size)
			alloc_size = mld->size - copied;

		skb = alloc_skb(alloc_size, GFP_ATOMIC);
		if (!skb) {
			skb_queue_purge(&iod->sk_rx_q);
			mif_err("ERR! alloc_skb fail, purged skb_rx_q\n");
			return -ENOMEM;
		}

		memcpy(skb_put(skb, alloc_size), mld->base + copied, alloc_size);
		copied += alloc_size;

		/* Record the IO device and the link device into the &skb->cb */
		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = ld;

		skbpriv(skb)->lnk_hdr = false;
		skbpriv(skb)->sipc_ch = iod->id;

		ret = iod->recv_skb_single(iod, ld, skb);
		if (unlikely(ret < 0)) {
			struct modem_ctl *mc = ld->mc;
			mif_err_limited("%s: %s<-%s: %s->recv_skb fail (%d)\n",
				ld->name, iod->name, mc->name, iod->name, ret);
			dev_kfree_skb_any(skb);
			return ret;
		}
	}

	mif_info("Complete! (%lu bytes)\n", copied);
	return 0;
}

static void modem_pci_close_tx(struct link_device *ld)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	unsigned long flags;

	spin_lock_irqsave(&mld->state_lock, flags);
	mld->state = LINK_STATE_OFFLINE;
	spin_unlock_irqrestore(&mld->state_lock, flags);

	if (timer_pending(&mld->crash_ack_timer))
		del_timer(&mld->crash_ack_timer);

	stop_net_ifaces(ld);
	purge_txq(mld);
}

#endif

#define MIF_BOOT_NOTI_TIMEOUT	(10 * HZ)

/*============================================================================*/
/* not in use */
static int modem_pci_ioctl(struct link_device *ld, struct io_device *iod,
		       unsigned int cmd, unsigned long arg)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct modem_ctl *mc = ld->mc;
	u8 *bar2_base = mld->base - mld->bar2_offset;
	unsigned long remain;
	int cnt = 0;

	mif_err("%s: cmd 0x%08X\n", ld->name, cmd);

	switch (cmd) {
	case IOCTL_MODEM_WAIT_CLEAR_COMMAND:
		while (*(u32*)(bar2_base + 0x80004) && cnt++ < 20) {
			mif_info("wait for clearing command flag, cnt: %d, 0x%X\n",
					cnt, *((u32*)(bar2_base + 0x80004)));
			msleep(100);
			if (cnt >= 20)
				panic("command flag has not been cleared");
		}
		break;

	case IOCTL_MODEM_WAIT_BOOT_NOTI:
		reinit_completion(&mc->boot_noti);
		remain = wait_for_completion_timeout(&mc->boot_noti, MIF_BOOT_NOTI_TIMEOUT);
		if (remain == 0)
			panic("didn't get boot noti");
		mif_info("got boot noti\n");
		break;

	default:
		mif_err("%s: ERR! invalid cmd 0x%08X\n", ld->name, cmd);
		return -EINVAL;
	}

	return 0;
}

#if 0
static void modem_pci_tx_state_handler(void *data)
{
	struct mem_link_device *mld = (struct mem_link_device *)data;
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	u16 int2ap_status;

	int2ap_status = mld->recv_cp2ap_status(mld);

	switch (int2ap_status & 0x0010) {
	case MASK_TX_FLOWCTL_SUSPEND:
		if (!chk_same_cmd(mld, int2ap_status))
			tx_flowctrl_suspend(mld);
		break;

	case MASK_TX_FLOWCTL_RESUME:
		if (!chk_same_cmd(mld, int2ap_status))
			tx_flowctrl_resume(mld);
		break;

	default:
		break;
	}

	if (unlikely(!rx_possible(mc))) {
		mif_err("%s: ERR! %s.state == %s\n", ld->name, mc->name,
			mc_state(mc));
		return;
	}
}
#endif

#if 0
static void modem_pci_cp2ap_wakelock_handler(void *data)
{
	struct mem_link_device *mld = (struct mem_link_device *)data;
	unsigned int req;
	mif_err("%s\n", __func__);

	req = mbox_get_value(mld->mbx_cp2ap_wakelock);

	if (req == 0) {
	       if (wake_lock_active(&mld->cp_wakelock)) {
			wake_unlock(&mld->cp_wakelock);
			mif_err("cp_wakelock unlocked\n");
	       } else {
			mif_err("cp_wakelock already unlocked\n");
	       }
	} else if (req == 1) {
	       if (wake_lock_active(&mld->cp_wakelock)) {
			mif_err("cp_wakelock already unlocked\n");
	       } else {
			wake_lock(&mld->cp_wakelock);
			mif_err("cp_wakelock locked\n");
	       }
	} else {
		mif_err("unsupported request: cp_wakelock\n");
	}
}
#endif

#if 0
static void remap_4mb_map_to_ipc_dev(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct shmem_4mb_phys_map *map;
	struct mem_ipc_device *dev;

	map = (struct shmem_4mb_phys_map *)mld->base;

	/* magic code and access enable fields */
	mld->magic = (u32 __iomem *)&map->magic;
	mld->access = (u32 __iomem *)&map->access;

	set_magic(mld, 0xab);

	/* IPC_FMT */
	dev = &mld->ipc_dev[IPC_FMT];

	dev->id = IPC_FMT;
	strcpy(dev->name, "FMT");

	spin_lock_init(&dev->txq.lock);
	atomic_set(&dev->txq.busy, 0);
	dev->txq.head = &map->fmt_tx_head;
	dev->txq.tail = &map->fmt_tx_tail;
	dev->txq.buff = &map->fmt_tx_buff[0];
	dev->txq.size = SHM_4M_FMT_TX_BUFF_SZ;

	spin_lock_init(&dev->rxq.lock);
	atomic_set(&dev->rxq.busy, 0);
	dev->rxq.head = &map->fmt_rx_head;
	dev->rxq.tail = &map->fmt_rx_tail;
	dev->rxq.buff = &map->fmt_rx_buff[0];
	dev->rxq.size = SHM_4M_FMT_RX_BUFF_SZ;

	dev->msg_mask = MASK_SEND_FMT;
	dev->req_ack_mask = MASK_REQ_ACK_FMT;
	dev->res_ack_mask = MASK_RES_ACK_FMT;

	dev->skb_txq = &ld->sk_fmt_tx_q;
	dev->skb_rxq = &ld->sk_fmt_rx_q;

	dev->req_ack_cnt[TX] = 0;
	dev->req_ack_cnt[RX] = 0;

	mld->dev[IPC_FMT] = dev;

	/* IPC_RAW */
	dev = &mld->ipc_dev[IPC_RAW];

	dev->id = IPC_RAW;
	strcpy(dev->name, "RAW");

	spin_lock_init(&dev->txq.lock);
	atomic_set(&dev->txq.busy, 0);
	dev->txq.head = &map->raw_tx_head;
	dev->txq.tail = &map->raw_tx_tail;
	dev->txq.buff = &map->raw_tx_buff[0];
	dev->txq.size = SHM_4M_RAW_TX_BUFF_SZ;

	spin_lock_init(&dev->rxq.lock);
	atomic_set(&dev->rxq.busy, 0);
	dev->rxq.head = &map->raw_rx_head;
	dev->rxq.tail = &map->raw_rx_tail;
	dev->rxq.buff = &map->raw_rx_buff[0];
	dev->rxq.size = SHM_4M_RAW_RX_BUFF_SZ;

	dev->msg_mask = MASK_SEND_RAW;
	dev->req_ack_mask = MASK_REQ_ACK_RAW;
	dev->res_ack_mask = MASK_RES_ACK_RAW;

	dev->skb_txq = &ld->sk_raw_tx_q;
	dev->skb_rxq = &ld->sk_raw_rx_q;

	dev->req_ack_cnt[TX] = 0;
	dev->req_ack_cnt[RX] = 0;

	mld->dev[IPC_RAW] = dev;
}
#endif

static int pci_irq_affinity_set(struct mem_link_device *mld, unsigned int irq)
{
	if (!zalloc_cpumask_var(&mld->dmask, GFP_KERNEL))
		return -ENOMEM;
	if (!zalloc_cpumask_var(&mld->imask, GFP_KERNEL))
		return -ENOMEM;
	if (!zalloc_cpumask_var(&mld->tmask, GFP_KERNEL))
		return -ENOMEM;

#ifdef CONFIG_ARGOS
	/* Below hard-coded mask values should be removed later on.
	 * Like net-sysfs, argos module also should support sysfs knob,
	 * so that user layer must be able to control these cpu mask. */
#ifdef CONFIG_SCHED_HMP
	cpumask_copy(mld->dmask, &hmp_slow_cpu_mask);
#endif

	cpumask_or(mld->imask, mld->imask, cpumask_of(3));

	argos_irq_affinity_setup_label(irq, "PCI_IPC", mld->imask, mld->dmask);
#endif
	return 0;
}

static int modem_pci_rx_setup(struct link_device *ld)
{
	/*
	 * For distributing core overheads,
	 * it recommends to use WQ_UNBOUND option when wq were creating.
	 * if following legacy(without WQ_UNBOUND), the wq will be steered to the core
	 * which that are working by cpu_affinity setting already.
	 * (never distribute core overheads)
	 */
	ld->rx_wq = alloc_workqueue(
//			"mem_rx_work", WQ_HIGHPRI | WQ_CPU_INTENSIVE, 1);
			"mem_rx_work", WQ_UNBOUND | WQ_HIGHPRI | WQ_CPU_INTENSIVE, 1);
	if (!ld->rx_wq) {
		mif_err("%s: ERR! fail to create rx_wq\n", ld->name);
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&ld->rx_delayed_work, link_to_demux_work);

	return 0;
}
#endif

struct link_device *pci_create_link_device(struct platform_device *pdev)
{
	struct modem_data *modem;
	struct mem_link_device *mld;
	struct link_device *ld;

	mif_err("+++\n");

	if (!pm_enable) {
		/*
		 pm_enable == 0, it means PM must be turn off
		 */
		wake_lock_init(&test_wlock, WAKE_LOCK_SUSPEND, "KTG_PM_LOCK");
		if (!wake_lock_active(&test_wlock))
			wake_lock(&test_wlock);
	}

	/**
	 * Get the modem (platform) data
	 */
	modem = (struct modem_data *)pdev->dev.platform_data;
	if (!modem) {
		mif_err("ERR! modem == NULL\n");
		return NULL;
	}

	if (modem->ipc_version < SIPC_VER_50) {
		mif_err("%s<->%s: ERR! IPC version %d < SIPC_VER_50\n",
			modem->link_name, modem->name, modem->ipc_version);
		return NULL;
	}

	mif_err("MODEM:%s LINK:%s\n", modem->name, modem->link_name);

	/*
	** Alloc an instance of mem_link_device structure
	*/
	mld = kzalloc(sizeof(struct mem_link_device), GFP_KERNEL);
	if (!mld) {
		mif_err("%s<->%s: ERR! mld kzalloc fail\n",
			modem->link_name, modem->name);
		return NULL;
	}

	/*
	** Retrieve modem-specific attributes value
	*/
	mld->attrs = modem->link_attrs;

	/*====================================================================*\
		Initialize "memory snapshot buffer (MSB)" framework
	\*====================================================================*/
	if (msb_init() < 0) {
		mif_err("%s<->%s: ERR! msb_init() fail\n",
			modem->link_name, modem->name);
		goto error;
	}

	/*====================================================================*\
		Set attributes as a "link_device"
	\*====================================================================*/
	ld = &mld->link_dev;

	ld->name = "link_pci";

	mif_err("ld name: %s(ld addr: %p) (mld addr: %p)\n",
		ld->name, ld, mld);

#if 1
	if (mld->attrs & LINK_ATTR(LINK_ATTR_SBD_IPC)) {
		mif_err("%s<->%s: LINK_ATTR_SBD_IPC\n", ld->name, modem->name);
		ld->sbd_ipc = true;
	}

	if (mld->attrs & LINK_ATTR(LINK_ATTR_IPC_ALIGNED)) {
		mif_err("%s<->%s: LINK_ATTR_IPC_ALIGNED\n",
			ld->name, modem->name);
		ld->aligned = true;
	}

	ld->ipc_version = modem->ipc_version;

	ld->mdm_data = modem;

	/*
	Set up link device methods
	*/
	ld->ioctl = modem_pci_ioctl;

	ld->init_comm = modem_pci_init_comm;
	ld->terminate_comm = modem_pci_terminate_comm;
	ld->init_descriptor= modem_pci_init_descriptor;
	ld->send = modem_pci_send;

	ld->boot_on = modem_pci_boot_on;
	if (mld->attrs & LINK_ATTR(LINK_ATTR_MEM_BOOT)) {
		if (mld->attrs & LINK_ATTR(LINK_ATTR_XMIT_BTDLR))
			ld->xmit_boot = modem_pci_xmit_boot;
		ld->dload_start = modem_pci_start_download;
		ld->firm_update = modem_pci_update_firm_info;
		ld->security_req = modem_pci_security_request;
	}

	ld->shmem_dump = modem_pci_full_dump;
	ld->force_dump = modem_pci_force_dump;

	if (mld->attrs & LINK_ATTR(LINK_ATTR_MEM_DUMP))
		ld->dump_start = modem_pci_start_upload;

	ld->close_tx = modem_pci_close_tx;

	INIT_LIST_HEAD(&ld->list);

	skb_queue_head_init(&ld->sk_fmt_tx_q);
	skb_queue_head_init(&ld->sk_raw_tx_q);

	skb_queue_head_init(&ld->sk_fmt_rx_q);
	skb_queue_head_init(&ld->sk_raw_rx_q);

	spin_lock_init(&ld->netif_lock);
	atomic_set(&ld->netif_stopped, 0);
	ld->tx_flowctrl_mask = 0;
	init_completion(&ld->raw_tx_resumed);

	if (modem_pci_rx_setup(ld) < 0)
		goto error;

	if (mld->attrs & LINK_ATTR(LINK_ATTR_DPRAM_MAGIC)) {
		mif_err("%s<->%s: LINK_ATTR_DPRAM_MAGIC\n",
			ld->name, modem->name);
		mld->dpram_magic = true;
	}
#ifdef CONFIG_LINK_CONTROL_MSG_IOSM
	mld->iosm = true;
	mld->cmd_handler = iosm_event_bh;
	INIT_WORK(&mld->iosm_w, iosm_event_work);
#else
	mld->cmd_handler = modem_pci_cmd_handler;
#endif

	spin_lock_init(&mld->state_lock);
	mld->state = LINK_STATE_OFFLINE;
#endif

#if 1
	/*
	** Initialize variables for TX & RX
	*/
	msb_queue_head_init(&mld->msb_rxq);
	msb_queue_head_init(&mld->msb_log);

#if 1
	/*
	 * Recently, Tasklet is not fit on this kinda massive packet system.
	 * So now we are using workqueue instead of it,
	 * We will compare the performance between tasklet and workqueue
	 * after affinity cpu lock for tasklet later.
	 */
	tasklet_init(&mld->rx_tsk, modem_pci_rx_task, (unsigned long)mld);
#endif
	hrtimer_init(&mld->tx_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mld->tx_timer.function = tx_timer_func;

	INIT_WORK(&mld->page_reclaim_work, modem_pci_oom_handler_work);

	/*
	** Initialize variables for CP booting and crash dump
	*/
	INIT_DELAYED_WORK(&mld->udl_rx_dwork, udl_rx_work);

	mld->pktlog = create_pktlog("modem_pci");
	if (!mld->pktlog)
		mif_err("packet log device create fail\n");
	
#endif
	modem_pcie_init(mld);

	if (linkdev_change_sysfs_register(&pdev->dev))
		mif_err("WARN! linkdev sysfs create failed\n");

	mif_err("---\n");

	return ld;

error:
	kfree(mld);
	mif_err("xxx\n");

	return NULL;
}

void modem_pcie_link_state_cb(struct exynos_pcie_notify *notify)
{
	struct modem_pcie_dev_info *mpdev_info;
	struct mem_link_device *mld;

	if (NULL == notify || NULL == notify->data) {
		mif_err("Incomplete handle received!\n");
		return;
	}

	mpdev_info = notify->data;

	mld = container_of(mpdev_info, struct mem_link_device, modem_pcie_dev);

	switch (notify->event) {
	case EXYNOS_PCIE_EVENT_LINKDOWN:
		mif_err("<KTG> PCIE link down event\n");
		break;
	case EXYNOS_PCIE_EVENT_LINKUP:
		mif_err("<KTG> PCIE link up event\n");
		break;
	case EXYNOS_PCIE_EVENT_WAKEUP:
		mif_err("<KTG> PCIE wake up event\n");
		break;
	default:
		break;
	}
}

#define REG_MAP(pa, size)   ioremap_nocache((unsigned long)(pa), (unsigned long)(size))

#ifdef CONFIG_PHYS_ADDR_T_64BIT
#define PRINTF_RESOURCE "0x%016llx"
#else
#define PRINTF_RESOURCE "0x%08x"
#endif

static int pcie_dev_init(struct pci_dev *pdev,
			 struct modem_pcie_dev_info *mpdev_info)
{
	int i;
	unsigned int bar_usage_flag = MODEM_PCI_BAR_USAGE;

	/* Enable the device */
	if (pci_enable_device(pdev)) {
		mif_err("Cannot enable PCI device!\n");
		return -ENODEV;
	}

	/* PCI Bar addr */
	for (i = 0; i < MAX_BAR_NUM; i++)
	{
		if (bar_usage_flag & (1 << i)) {
			mpdev_info->bar_base[i] = ioremap_nocache(pci_resource_start(pdev, i),
					pci_resource_len(pdev, i));
			mpdev_info->bar_end[i] = mpdev_info->bar_base[i] +
				pci_resource_len(pdev, i);
			if (!mpdev_info->bar_base[i]) {
				mif_err("Failed to register for pcie resources[BAR %d]\n",
									i);
				return -ENODEV;
			}

			if (pci_request_region(pdev, i, MODEM_PCI_BAR_NAME"")) {
				mif_err("Could not request BAR %d region!\n", i);
			}

			mif_err("Modem PCI BAR [%d] (%s) : start: %p, end: %p\n",
						i,
						pdev->driver->name,
						mpdev_info->bar_base[i],
						mpdev_info->bar_end[i]);
		}
	}

	mpdev_info->pdev = pdev;

	if (pdev->vendor != MODEM_PCIE_VENDOR_ID) {
		mif_err("chipmatch failed!\n");

		pci_disable_device(pdev);
		return -ENODEV;
	}

	/* Set PCI master (for DMA) */
	pci_set_master(pdev);

	return 0;
}

static int modem_pcie_probe(struct pci_dev *pdev,
			    const struct pci_device_id *pdev_id)
{
	struct mem_link_device *mld;
	struct modem_pcie_dev_info *mpdev_info;
	struct link_device *ld;
	int ret;
	int err;

	mif_err("pci bus %X, slot %X (v_i:%x, d_i:%x)\n",
		pdev->bus->number, PCI_SLOT(pdev->devfn), pdev_id->vendor, pdev_id->device);

	mld = container_of(pdev->driver, struct mem_link_device, pdev_drv);
	ld = &mld->link_dev;

	mld->pdev = pdev;

	mpdev_info = &mld->modem_pcie_dev;

#if 1
	mpdev_info->mp_link_evt.events =
			(EXYNOS_PCIE_EVENT_LINKDOWN | EXYNOS_PCIE_EVENT_LINKUP |
			 EXYNOS_PCIE_EVENT_WAKEUP);
	mpdev_info->mp_link_evt.user = pdev;
	mpdev_info->mp_link_evt.callback = modem_pcie_link_state_cb;
	mpdev_info->mp_link_evt.notify.data = mpdev_info;
	ret = exynos_pcie_register_event(&mpdev_info->mp_link_evt);
	if (ret)
		mif_err("Failed to register for link notifications (%d)\n", ret);
#endif
	if (pcie_dev_init(pdev, mpdev_info)) {
		mif_err("Modem PCIe Enumeration failed\n");
		return -ENODEV;
	}

#if 1
	/**
	 * Initialize Modem PCI maps for IPC (physical map -> logical map)
	 */
	mld->bar0 = mpdev_info->bar_base[0];
	mld->bar0_size = mpdev_info->bar_end[0] - mpdev_info->bar_base[0];

	mld->bar2_offset = (mld->bar0[3]<<24) | (mld->bar0[2]<<16) | (mld->bar0[1]<<8) | mld->bar0[0];
	mif_err("BAR2 start offset:0x%X\n", mld->bar2_offset);

	mld->base = mpdev_info->bar_base[2] + mld->bar2_offset;
	mld->size = mpdev_info->bar_end[2] - (mpdev_info->bar_base[2] + mld->bar2_offset);

	if (!mld->base) {
		mif_err("Failed to vmap ipc_region\n");
		goto error;
	}
	mif_err("ipc_base=%p, ipc_size=%lu\n",
		mld->base, (unsigned long)mld->size);

	mld->boot_base = mpdev_info->bar_base[4];
	mld->boot_size = mpdev_info->bar_end[4] - mpdev_info->bar_base[4];
	if (!mld->boot_base) {
		mif_err("Failed to vmap boot_region\n");
		goto error;
	}
	mif_err("boot_base=%p, boot_size=%lu\n",
			mld->boot_base, (unsigned long)mld->boot_size);

	if (ld->sbd_ipc) {
 		size_t t_size;

		if (request_irq(pdev->irq, modem_pci_irq_handler,
				IRQF_SHARED,
				"modem_pci_isr", mld) < 0) {
			mif_err("PCIE request_irq() failed\n");
			goto error;
		} else {
			mif_err("PCIE request_irq ok(%d)\n", pdev->irq);
		}

		hrtimer_init(&mld->sbd_tx_timer,
				CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		mld->sbd_tx_timer.function = sbd_tx_timer_func;

		err = create_pci_sbd_link_device(ld,
				&mld->sbd_link_dev, mld->base, mld->size);
		if (err < 0)
			goto error;

		t_size = (mpdev_info->bar_end[0] - mpdev_info->bar_base[0]);
		if (t_size > DESC_RGN_SIZE)
			t_size = DESC_RGN_SIZE;
		err = init_pci_sbd_intr(&mld->sbd_link_dev, mpdev_info->bar_base[0], t_size);
		if (err < 0)
			goto error;

		mif_err("pci(sbd_intr) base=%p, size=%lu\n", mpdev_info->bar_base[0], t_size);

		if (mld->attrs & LINK_ATTR(LINK_ATTR_IPC_ALIGNED))
			ld->aligned = true;
		else
			ld->aligned = false;

		sbd_activate(&mld->sbd_link_dev);

#ifdef CONFIG_LINK_DEVICE_NAPI
		INIT_WORK(&mld->polling_timer, polling_timer_func);

		atomic_set(&test_napi, 1);
#endif

	}

	pci_irq_affinity_set(mld, pdev->irq);
#endif

	pci_set_drvdata(pdev, mpdev_info);

#if 1
	//For ipc test
	ld->mc->phone_state = STATE_ONLINE;
#endif

	return 0;
#if 1
error:
	kfree(mld);
	mif_err("xxx\n");
	return -ENODEV;
#endif
}

void modem_pcie_remove(struct pci_dev *pdev)
{
	exynos_pcie_poweroff(MODEM_PCIE_CH_NUM);

	return;
}

struct pci_driver modem_pcie_driver = {
	.name = "modem_pcie_drv",
	.id_table = modem_pcie_device_id,
	.probe = modem_pcie_probe,
	.remove = modem_pcie_remove,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		.name = "modem_pcie_drv",
		.of_match_table = of_match_ptr(modem_pcie_match),
	},
};

int modem_pcie_init(struct mem_link_device *mld)
{
	mld->pdev_drv = modem_pcie_driver;

	if (pci_register_driver(&mld->pdev_drv)) {
		mif_err("pci register failed!!\n");

		return -EIO;
	}

	mif_info("PCI drv(%s) registered\n", mld->pdev_drv.name);

	return 0;
}

