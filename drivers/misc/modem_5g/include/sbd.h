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

#ifndef __MODEM_INCLUDE_SBD_H__
#define __MODEM_INCLUDE_SBD_H__

/**
@file		sbd.h
@brief		header file for shared buffer descriptor (SBD) architecture
		designed by MIPI Alliance
@date		2014/02/05
@author		Hankook Jang (hankook.jang@samsung.com)
*/

#ifdef GROUP_MEM_LINK_SBD
/**
@addtogroup group_mem_link_sbd
@{
*/

#include <linux/types.h>
#include <linux/kfifo.h>
#include "../modem_v1.h"

#include "link_device_memory_config.h"
#include "circ_queue.h"

/*
Abbreviations
=============
SB, sb		Shared Buffer
SBD, sbd	Shared Buffer Descriptor
SBDV, sbdv	Shared Buffer Descriptor Vector (Array)
--------
RB, rb		Ring Buffer
RBO, rbo	Ring Buffer Offset (offset of an RB)
RBD, rbd	Ring Buffer Descriptor (descriptor of an RB)
RBDO, rbdo	Ring Buffer Descriptor Offset (offset of an RBD)
--------
RBP, rbp	Ring Buffer Pointer (read pointer, write pointer)
RBPS, rbps	Ring Buffer Pointer Set (set of RBPs)
--------
CH, ch		Channel
CHD, chd	Channel Descriptor
--------
desc		descriptor
*/

#define CMD_DESC_RGN_OFFSET	0
#define CMD_DESC_RGN_SIZE	SZ_64K
#define CMD_PCI_DESC_RGN_SIZE	SZ_4K

#define CTRL_RGN_OFFSET	(CMD_DESC_RGN_OFFSET)
#define CTRL_RGN_SIZE	(1 * SZ_1K)

#define CMD_RGN_OFFSET	(CTRL_RGN_OFFSET + CTRL_RGN_SIZE)
#define CMD_RGN_SIZE	(1 * SZ_1K)

#define DESC_RGN_OFFSET	(CMD_RGN_OFFSET + CMD_RGN_SIZE)
#define DESC_RGN_SIZE	(CMD_DESC_RGN_SIZE - CTRL_RGN_SIZE - CMD_RGN_SIZE)

#define BUFF_RGN_OFFSET	(CMD_DESC_RGN_SIZE)

/**
@brief		Priority for QoS(Quality of Service)
*/
enum qos_prio {
	QOS_HIPRIO = 10,
	QOS_NORMAL,
	QOS_MAX_PRIO,
};

#ifdef CONFIG_LINK_DEVICE_PCI
/**
@brief		SBD uDMA Descriptor
		(i.e. DMA Descriptor)
*/

#define HIGH_WORD(_x) ((u32)((((u64)(_x)) >> 32) & 0xFFFFFFFF))
#define LOW_WORD(_x) ((u32)(((u64)(_x)) & 0xFFFFFFFF))

#define SBD_DMA_SKB_MAX_SIZE	1502	//SZ_8K

#define PCI_BAR0_DL0_IRQ	0xC0	//0x300

struct dma_buffer_match {
	void *va_buffer;
};

//36 bytes
struct __packed sbd_dma_desc {
#if 1
	u32 ext_lo_addr;
	u32 ext_hi_addr;
	u32 size;
#else
	u32 sys_lo_addr;
	u32 sys_attr;
	u32 ext_lo_addr;
	u32 ext_hi_addr;
	u32 ext_attr;
	u32 ext_attr_hi;

#ifdef CONFIG_CPU_BIG_ENDIAN
	u32 ctrl:8;
	u32 size:24;
#else
	u32 size:24;
	u32 ctrl:8;
#endif

	u32 status;
	u32 next;
#endif
};

#define INTR_TRIGGER	(0x80000000)

struct __packed sbd_intr {
	u32 wp_reserved[3];

	u32 wp_intr;
#if 0
#ifdef CONFIG_CPU_BIG_ENDIAN
	u32 wp_intr:1;
	u32 wp_intr_reserved:31;
#else
	u32 wp_intr_reserved:31;
	u32 wp_intr:1;
#endif
#endif

	u32 rp_reserved[3];

	u32 rp_intr;
#if 0
#ifdef CONFIG_CPU_BIG_ENDIAN
	u32 rp_intr:1;
	u32 rp_intr_reserved:31;
#else
	u32 rp_intr_reserved:31;
	u32 rp_intr:1;
#endif
#endif
};

#endif

/**
@brief		SBD Ring Buffer Descriptor
		(i.e. IPC Channel Descriptor Structure in MIPI LLI_IPC_AN)
*/
struct __packed sbd_rb_desc {
	/*
	ch		Channel number defined in the Samsung IPC specification
	--------
	reserved
	*/
	u16 ch;
	u16 reserved;

	/*
	direction	0 (UL, TX, AP-to-CP), 1 (DL, RX, CP-to-AP)
	--------
	signaling	0 (polling), 1 (interrupt)
	*/
	u16 direction;
	u16 signaling;

	/*
	Mask to be written to the signal register (viz. 1 << @@id)
	(i.e. write_signal)
	*/
	u32 sig_mask;

	/*
	length		Length of an SBD Ring Buffer
	--------
	id		(1) ID for a link channel that consists of an RB pair
			(2) Index into each array in the set of RBP arrays
			    N.B. set of RBP arrays =
			    {[ul_rp], [ul_wp], [dl_rp], [dl_wp]}
	*/
	u16 length;
	u16 id;

	/*
	buff_size	Size of each data buffer in an SBD RB (default 2048)
	--------
	payload_offset	Offset of the payload in each data buffer (default 0)
	*/
	u16 buff_size;
	u16 payload_offset;
};

/**
@brief		SBD Channel Descriptor
*/
struct __packed sbd_rb_channel {
	u32 ul_rbd_offset;	/* UL RB Descriptor Offset	*/
	u32 ul_sbdv_offset;	/* UL SBD Vectors's Offset	*/
	u32 dl_rbd_offset;	/* DL RB Descriptor's Offset	*/
	u32 dl_sbdv_offset;	/* DL SBD Vector's Offset	*/
};

/**
@brief		SBD Global Descriptor
*/
struct __packed sbd_global_desc {
	/*
	Version
	*/
	u32 version;

	/*
	Number of link channels
	*/
	u32 num_channels;

	/*
	Offset of the array of "SBD Ring Buffer Pointers Set" in SHMEM
	*/
	u32 rbps_offset;

	/*
	Array of SBD channel descriptors
	*/
	struct sbd_rb_channel rb_ch[MAX_LINK_CHANNELS];

	/*
	Array of SBD ring buffer descriptor pairs
	*/
	struct sbd_rb_desc rb_desc[MAX_LINK_CHANNELS][ULDL];
};

enum sbd_global_desc_offset {
	SBD_GB_VER = 0,
	SBD_GB_NUM_CH = SBD_GB_VER + 4,
	SBD_GB_RBPS_OFFSET = SBD_GB_NUM_CH + 4,
	SBD_GB_RB_CH = SBD_GB_RBPS_OFFSET + 4,
	SBD_GB_RB_DESC = SBD_GB_RB_CH + (sizeof(struct sbd_rb_channel) * MAX_LINK_CHANNELS),
};


/**
@brief		SBD ring buffer (with logical view)
@remark		physical SBD ring buffer
		= {length, *rp, *wp, offset array, size array}
*/
struct sbd_ring_buffer {
	/*
	Spin-lock for each SBD RB
	*/
	spinlock_t lock;

	/*
	Pointer to the "SBD link" device instance to which an RB belongs
	*/
	struct sbd_link_device *sl;

	/*
	Pointer to the "zerocopy_adaptor" device instance to which an RB belongs
	*/
	struct zerocopy_adaptor *zdptr;

	/*
	UL/DL socket buffer queues
	*/
	struct sk_buff_head skb_q;

	/*
	Whether or not link-layer header is used
	*/
	bool lnk_hdr;

	/*
	Whether or not zerocopy is used
	*/
	bool zerocopy;

	/*
	Variables for receiving a frame with the SIPC5 "EXT_LEN" attribute
	(With SBD architecture, a frame with EXT_LEN can be scattered into
	 consecutive multiple data buffer slots.)
	*/
	bool more;
	unsigned int total;
	unsigned int rcvd;

	/*
	Link ID, SIPC channel, and IPC direction
	*/
	u16 id;			/* for @desc->id			*/
	u16 ch;			/* for @desc->ch			*/
	u16 dir;		/* for @desc->direction			*/
	u16 len;		/* for @desc->length			*/
	u16 buff_size;		/* for @desc->buff_size			*/
	u16 payload_offset;	/* for @desc->payload_offset		*/

	/*
	Pointer to the array of pointers to each data buffer
	*/
	u8 **buff;

	/*
	Pointer to the data buffer region in SHMEM
	*/
	u8 *buff_rgn;

	/*
	Pointers to variables in the shared region for a physical SBD RB
	*/
	u16 *rp;		/* sl->rp[@dir][@id]			*/
	u16 *wp;		/* sl->wp[@dir][@id]			*/
	u32 *addr_v;		/* Vector array of offsets		*/
	u32 *size_v;		/* Vector array of sizes		*/

#ifdef CONFIG_LINK_DEVICE_PCI
	u32 *pbuff_rgn;

	struct sk_buff_head sk_buffers;
	struct sbd_dma_desc *dma_desc;
	struct sbd_dma_desc *dma_local_desc;

	u16 *local_rp;
	u16 *local_wp;

	struct dma_buffer_match *dma_buffer;

	struct sbd_intr *sbd_intr;
#endif

	/*
	Pointer to the IO device and the link device to which an SBD RB belongs
	*/
	struct io_device *iod;
	struct link_device *ld;

	/* Flow control */
	atomic_t busy;
};

struct sbd_link_attr {
	/*
	Link ID and SIPC channel number
	*/
	u16 id;
	u16 ch;

	/*
	Whether or not link-layer header is used
	*/
	bool lnk_hdr;

	/*
	Length of an SBD RB
	*/
	unsigned int rb_len[ULDL];

	/*
	Size of the data buffer for each SBD in an SBD RB
	*/
	unsigned int buff_size[ULDL];

	/*
	Bool variable to check if SBD ipc device supports zerocopy
	*/
	bool zerocopy;
};

struct zerocopy_adaptor {
	/*
	Spin-lock for each zerocopy_adaptor
	*/
	spinlock_t lock;

	/*
	Spin-lock for kfifo
	*/
	spinlock_t lock_kfifo;

	/*
	SBD ring buffer that matches this zerocopy_adaptor
	*/
	struct sbd_ring_buffer *rb;

	/*
	Variables to manage previous rp
	*/
	u16 pre_rp;

	/*
	Pointers to variables in the shared region for a physical SBD RB
	*/
	u16 *rp;
	u16 *wp;
	u16 len;

	/*
	Pointer to kfifo for saving dma_addr
	*/
	struct kfifo fifo;

	/*
	Timer for when buffer pool is full
	*/
	struct hrtimer datalloc_timer;
};

struct sbd_ipc_device {
	/*
	Pointer to the IO device to which an SBD IPC device belongs
	*/
	struct io_device *iod;

	/*
	SBD IPC device ID == Link ID --> rb.id
	*/
	u16 id;

	/*
	SIPC Channel ID --> rb.ch
	*/
	u16 ch;

	atomic_t config_done;

	/*
	UL/DL SBD RB pair in the kernel space
	*/
	struct sbd_ring_buffer rb[ULDL];

	/*
	Bool variable to check if SBD ipc device supports zerocopy
	*/
	bool zerocopy;

	/*
	Pointer to Zerocopy adaptor : memory is allocated for UL/DL zerocopy_adaptor
	*/
	struct zerocopy_adaptor *zdptr;
};

struct sbd_link_device {
	/*
	Pointer to the link device to which an SBD link belongs
	*/
	struct link_device *ld;

	/*
	Flag for checking whether or not an SBD link is active
	*/
	atomic_t active;

	/*
	Version of SBD IPC
	*/
	unsigned int version;

	/*
	Start address of SHMEM
	@shmem = SHMEM.VA
	*/
	u8 *shmem;
	unsigned int shmem_size;

#ifdef CONFIG_LINK_DEVICE_PCI
	struct pci_dev *pdev;

	/* start address of bar0 */
	u8 *shmem_intr;
	u32 shmem_intr_size;
	int tx_confirm_irq;

	/* start address of bar4 */
	u8 *shmem_bar4;
	unsigned int shmem_bar4_size;

	u32 intr_alloc_offset;

	u16 *local_rbps;
	u16 *local_rp[ULDL];
	u16 *local_wp[ULDL];

	struct dentry *dbgfs_dir;
	struct dentry *pci_bar4_map_dbgfs;
	struct dentry *pci_bar2_map_dbgfs;
	struct dentry *pci_bar0_map_dbgfs;

	struct hrtimer sbd_test_timer;

	struct work_struct           rx_alloc_skb_work;
	struct workqueue_struct      *rx_alloc_work_queue;

	struct work_struct           tx_alloc_skb_work;
	struct workqueue_struct      *tx_alloc_work_queue;
#endif

	/*
	Variables for DESC & BUFF allocation management
	*/
	unsigned int desc_alloc_offset;
	unsigned int buff_alloc_offset;

	/*
	The number of link channels for AP-CP IPC
	*/
	unsigned int num_channels;

	/*
	Table of link attributes
	*/
	struct sbd_link_attr link_attr[MAX_LINK_CHANNELS];

	/*
	Logical IPC devices
	*/
	struct sbd_ipc_device ipc_dev[MAX_LINK_CHANNELS];

	/*
	(1) Conversion tables from "Link ID (ID)" to "SIPC Channel Number (CH)"
	(2) Conversion tables from "SIPC Channel Number (CH)" to "Link ID (ID)"
	*/
	u16 id2ch[MAX_LINK_CHANNELS];
	u16 ch2id[MAX_SIPC_CHANNELS];

	/*
	Pointers to each array of arrays of SBD RB Pointers,
	viz. rp[UL] = pointer to ul_rp[]
	     rp[DL] = pointer to dl_rp[]
	     wp[UL] = pointer to ul_wp[]
	     wp[DL] = pointer to dl_wp[]
	*/
	u16 *rp[ULDL];
	u16 *wp[ULDL];

	/*
	Above are variables for managing and controlling SBD IPC
	========================================================================
	Below are pointers to the descriptor sections in SHMEM
	*/

	/*
	Pointer to the SBD global descriptor header
	*/
	struct sbd_global_desc *g_desc;

	u16 *rbps;

	unsigned long rxdone_mask;

	bool reset_zerocopy_done;
};

static inline void sbd_activate(struct sbd_link_device *sl)
{
	if (sl)
		atomic_set(&sl->active, 1);
}

static inline void sbd_deactivate(struct sbd_link_device *sl)
{
	if (sl)
		atomic_set(&sl->active, 0);
}

static inline bool sbd_active(struct sbd_link_device *sl)
{
	if (!sl)
		return false;
	return atomic_read(&sl->active) ? true : false;
}

static inline u16 sbd_ch2id(struct sbd_link_device *sl, u16 ch)
{
	return sl->ch2id[ch];
}

static inline u16 sbd_id2ch(struct sbd_link_device *sl, u16 id)
{
	return sl->id2ch[id];
}

static inline struct sbd_ipc_device *sbd_ch2dev(struct sbd_link_device *sl,
						u16 ch)
{
	u16 id = sbd_ch2id(sl, ch);
	return (id < MAX_LINK_CHANNELS) ? &sl->ipc_dev[id] : NULL;
}

static inline struct sbd_ipc_device *sbd_id2dev(struct sbd_link_device *sl,
						u16 id)
{
	return (id < MAX_LINK_CHANNELS) ? &sl->ipc_dev[id] : NULL;
}

static inline struct sbd_ring_buffer *sbd_ch2rb(struct sbd_link_device *sl,
						unsigned int ch,
						enum direction dir)
{
	u16 id = sbd_ch2id(sl, ch);
	return (id < MAX_LINK_CHANNELS) ? &sl->ipc_dev[id].rb[dir] : NULL;
}

static inline struct sbd_ring_buffer *sbd_ch2rb_with_skb(struct sbd_link_device *sl,
						unsigned int ch,
						enum direction dir,
						struct sk_buff *skb)
{
	u16 id;

#ifdef CONFIG_MODEM_IF_QOS
	if (sipc_ps_ch(ch))
		ch = (skb && skb->queue_mapping == 1) ? QOS_HIPRIO : QOS_NORMAL;
#endif

	id = sbd_ch2id(sl, ch);
	return (id < MAX_LINK_CHANNELS) ? &sl->ipc_dev[id].rb[dir] : NULL;
}

static inline struct sbd_ring_buffer *sbd_id2rb(struct sbd_link_device *sl,
						unsigned int id,
						enum direction dir)
{
	return (id < MAX_LINK_CHANNELS) ? &sl->ipc_dev[id].rb[dir] : NULL;
}

static inline bool zerocopy_adaptor_empty(struct zerocopy_adaptor *zdptr)
{
	return circ_empty(zdptr->pre_rp, *zdptr->rp);
}

static inline bool rb_empty(struct sbd_ring_buffer *rb)
{
	BUG_ON(!rb);

	if (rb->zdptr)
		return zerocopy_adaptor_empty(rb->zdptr);
	else
		return circ_empty(*rb->rp, *rb->wp);
}

static inline unsigned int zerocopy_adaptor_space(struct zerocopy_adaptor *zdptr)
{
	return circ_get_space(zdptr->len, *zdptr->wp, zdptr->pre_rp);
}

static inline unsigned int rb_space(struct sbd_ring_buffer *rb)
{
	BUG_ON(!rb);

	if (rb->zdptr)
		return zerocopy_adaptor_space(rb->zdptr);
	else
		return circ_get_space(rb->len, *rb->wp, *rb->rp);
}

static inline unsigned int zerocopy_adaptor_usage(struct zerocopy_adaptor *zdptr)
{
	return circ_get_usage(zdptr->len, *zdptr->rp, zdptr->pre_rp);
}

static inline unsigned int rb_usage(struct sbd_ring_buffer *rb)
{
	BUG_ON(!rb);

	if (rb->zdptr)
		return zerocopy_adaptor_usage(rb->zdptr);
	else
		return circ_get_usage(rb->len, *rb->wp, *rb->rp);
}

static inline unsigned int zerocopy_adaptor_full(struct zerocopy_adaptor *zdptr)
{
	return (zerocopy_adaptor_space(zdptr) == 0);
}

static inline unsigned int rb_full(struct sbd_ring_buffer *rb)
{
	BUG_ON(!rb);

	if (rb->zdptr)
		return zerocopy_adaptor_full(rb->zdptr);
	else
		return (rb_space(rb) == 0);
}

int create_sbd_link_device(struct link_device *ld, struct sbd_link_device *sl,
			   u8 *shmem_base, unsigned int shmem_size);

int init_sbd_link(struct sbd_link_device *sl);

int sbd_pio_tx(struct sbd_ring_buffer *rb, struct sk_buff *skb);
struct sk_buff *sbd_pio_rx_zerocopy_adaptor(struct sbd_ring_buffer *rb, int use_memcpy);
struct sk_buff *sbd_pio_rx(struct sbd_ring_buffer *rb);
int allocate_data_in_advance(struct zerocopy_adaptor *zdptr);
extern enum hrtimer_restart datalloc_timer_func(struct hrtimer *timer);

#define SBD_UL_LIMIT		16	/* Uplink burst limit */

#ifdef CONFIG_LINK_DEVICE_PCI
int create_pci_sbd_link_device(struct link_device *ld, struct sbd_link_device *sl,
			   u8 *shmem_base, unsigned int shmem_size);
int init_pci_sbd_intr(struct sbd_link_device *sl,
		u8 *base, u32 size);
int init_pci_sbd_bar4(struct sbd_link_device *sl, u8 *base, u32 size);
int init_pci_sbd_link(struct sbd_link_device *sl);

int pci_sbd_pio_tx(struct sbd_ring_buffer *rb, struct sk_buff *skb);
struct sk_buff *pci_sbd_pio_rx(struct sbd_ring_buffer *rb);
int pci_sbd_tx_clear(struct sbd_ring_buffer *rb);

int pci_dma_alloc(struct sbd_ring_buffer *rb);

void alloc_skb_work(struct work_struct *work);
#endif

/**
// End of group_mem_link_sbd
@}
*/
#endif

#endif
