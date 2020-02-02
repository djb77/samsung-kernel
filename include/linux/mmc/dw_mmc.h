/*
 * Synopsys DesignWare Multimedia Card Interface driver
 *  (Based on NXP driver for lpc 31xx)
 *
 * Copyright (C) 2009 NXP Semiconductors
 * Copyright (C) 2009, 2010 Imagination Technologies Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef LINUX_MMC_DW_MMC_H
#define LINUX_MMC_DW_MMC_H

#include <linux/scatterlist.h>
#include <linux/mmc/core.h>
#include <linux/dmaengine.h>
#include <linux/reset.h>
#include <linux/pm_qos.h>

#define MAX_MCI_SLOTS	2

enum dw_mci_state {
	STATE_IDLE = 0,
	STATE_SENDING_CMD,
	STATE_SENDING_DATA,
	STATE_DATA_BUSY,
	STATE_SENDING_STOP,
	STATE_DATA_ERROR,
	STATE_SENDING_CMD11,
	STATE_WAITING_CMD11_DONE,
};

enum {
	EVENT_CMD_COMPLETE = 0,
	EVENT_XFER_COMPLETE,
	EVENT_DATA_COMPLETE,
	EVENT_DATA_ERROR,
};

struct mmc_data;

enum {
	TRANS_MODE_PIO = 0,
	TRANS_MODE_IDMAC,
	TRANS_MODE_EDMAC
};

struct dw_mci_dma_slave {
	struct dma_chan *ch;
	enum dma_transfer_direction direction;
};

/**
 * struct dw_mci - MMC controller state shared between all slots
 * @lock: Spinlock protecting the queue and associated data.
 * @irq_lock: Spinlock protecting the INTMASK setting.
 * @regs: Pointer to MMIO registers.
 * @fifo_reg: Pointer to MMIO registers for data FIFO
 * @sg: Scatterlist entry currently being processed by PIO code, if any.
 * @sg_miter: PIO mapping scatterlist iterator.
 * @cur_slot: The slot which is currently using the controller.
 * @mrq: The request currently being processed on @cur_slot,
 *	or NULL if the controller is idle.
 * @cmd: The command currently being sent to the card, or NULL.
 * @data: The data currently being transferred, or NULL if no data
 *	transfer is in progress.
 * @stop_abort: The command currently prepared for stoping transfer.
 * @prev_blksz: The former transfer blksz record.
 * @timing: Record of current ios timing.
 * @use_dma: Whether DMA channel is initialized or not.
 * @using_dma: Whether DMA is in use for the current transfer.
 * @dma_64bit_address: Whether DMA supports 64-bit address mode or not.
 * @sg_dma: Bus address of DMA buffer.
 * @sg_cpu: Virtual address of DMA buffer.
 * @dma_ops: Pointer to platform-specific DMA callbacks.
 * @cmd_status: Snapshot of SR taken upon completion of the current
 * @ring_size: Buffer size for idma descriptors.
 *	command. Only valid when EVENT_CMD_COMPLETE is pending.
 * @dms: structure of slave-dma private data.
 * @phy_regs: physical address of controller's register map
 * @data_status: Snapshot of SR taken upon completion of the current
 *	data transfer. Only valid when EVENT_DATA_COMPLETE or
 *	EVENT_DATA_ERROR is pending.
 * @stop_cmdr: Value to be loaded into CMDR when the stop command is
 *	to be sent.
 * @dir_status: Direction of current transfer.
 * @tasklet: Tasklet running the request state machine.
 * @pending_events: Bitmask of events flagged by the interrupt handler
 *	to be processed by the tasklet.
 * @completed_events: Bitmask of events which the state machine has
 *	processed.
 * @state: Tasklet state.
 * @queue: List of slots waiting for access to the controller.
 * @bus_hz: The rate of @mck in Hz. This forms the basis for MMC bus
 *	rate and timeout calculations.
 * @current_speed: Configured rate of the controller.
 * @num_slots: Number of slots available.
 * @fifoth_val: The value of FIFOTH register.
 * @verid: Denote Version ID.
 * @dev: Device associated with the MMC controller.
 * @pdata: Platform data associated with the MMC controller.
 * @drv_data: Driver specific data for identified variant of the controller
 * @priv: Implementation defined private data.
 * @biu_clk: Pointer to bus interface unit clock instance.
 * @ciu_clk: Pointer to card interface unit clock instance.
 * @slot: Slots sharing this MMC controller.
 * @fifo_depth: depth of FIFO.
 * @data_shift: log2 of FIFO item size.
 * @part_buf_start: Start index in part_buf.
 * @part_buf_count: Bytes of partial data in part_buf.
 * @part_buf: Simple buffer for partial fifo reads/writes.
 * @push_data: Pointer to FIFO push function.
 * @pull_data: Pointer to FIFO pull function.
 * @quirks: Set of quirks that apply to specific versions of the IP.
 * @vqmmc_enabled: Status of vqmmc, should be true or false.
 * @irq_flags: The flags to be passed to request_irq.
 * @irq: The irq value to be passed to request_irq.
 * @sdio_id0: Number of slot0 in the SDIO interrupt registers.
 * @cmd11_timer: Timer for SD3.0 voltage switch over scheme.
 * @dto_timer: Timer for broken data transfer over scheme.
 *
 * Locking
 * =======
 *
 * @lock is a softirq-safe spinlock protecting @queue as well as
 * @cur_slot, @mrq and @state. These must always be updated
 * at the same time while holding @lock.
 *
 * @irq_lock is an irq-safe spinlock protecting the INTMASK register
 * to allow the interrupt handler to modify it directly.  Held for only long
 * enough to read-modify-write INTMASK and no other locks are grabbed when
 * holding this one.
 *
 * The @mrq field of struct dw_mci_slot is also protected by @lock,
 * and must always be written at the same time as the slot is added to
 * @queue.
 *
 * @pending_events and @completed_events are accessed using atomic bit
 * operations, so they don't need any locking.
 *
 * None of the fields touched by the interrupt handler need any
 * locking. However, ordering is important: Before EVENT_DATA_ERROR or
 * EVENT_DATA_COMPLETE is set in @pending_events, all data-related
 * interrupts must be disabled and @data_status updated with a
 * snapshot of SR. Similarly, before EVENT_CMD_COMPLETE is set, the
 * CMDRDY interrupt must be disabled and @cmd_status updated with a
 * snapshot of SR, and before EVENT_XFER_COMPLETE can be set, the
 * bytes_xfered field of @data must be written. This is ensured by
 * using barriers.
 */
struct dw_mci {
	spinlock_t		lock;
	spinlock_t		irq_lock;
	void __iomem		*regs;
	void __iomem		*fifo_reg;

	struct scatterlist	*sg;
	struct sg_mapping_iter	sg_miter;

	struct dw_mci_slot	*cur_slot;
	struct mmc_request	*mrq;
	struct mmc_command	*cmd;
	struct mmc_data		*data;
	struct mmc_command	stop_abort;
	unsigned int		prev_blksz;
	unsigned char		timing;
	struct workqueue_struct	*card_workqueue;

	/* DMA interface members*/
	int			use_dma;
	int			using_dma;
	int			dma_64bit_address;

	dma_addr_t		sg_dma;
	void			*sg_cpu;
	const struct dw_mci_dma_ops	*dma_ops;
	/* For idmac */
	unsigned int		ring_size;

	/* For edmac */
	struct dw_mci_dma_slave *dms;
	/* Registers's physical base address */
	resource_size_t		phy_regs;

	unsigned int            desc_sz;
	struct pm_qos_request   pm_qos_lock;
	struct delayed_work	qos_work;
	bool			qos_cntrl;
	u32			cmd_status;
	u32			data_status;
	u32			stop_cmdr;
	u32			dir_status;
	struct tasklet_struct	tasklet;
	u32			tasklet_state;
	struct work_struct	card_work;
	u32			card_detect_cnt;
	unsigned long		pending_events;
	unsigned long		completed_events;
	enum dw_mci_state	state;
	struct list_head	queue;

	u32			bus_hz;
	u32			current_speed;
	u32			num_slots;
	u32			fifoth_val;
	u32			cd_rd_thr;
	u16			verid;
	u16			data_offset;
	struct device		*dev;
	struct dw_mci_board	*pdata;
	const struct dw_mci_drv_data	*drv_data;
	void			*priv;
	struct clk		*biu_clk;
	struct clk		*ciu_clk;
	struct clk		*ciu_gate;
	atomic_t		biu_clk_cnt;
	atomic_t		ciu_clk_cnt;
	atomic_t		biu_en_win;
	atomic_t		ciu_en_win;
	struct dw_mci_slot	*slot[MAX_MCI_SLOTS];

	/* FIFO push and pull */
	int			fifo_depth;
	int			data_shift;
	u8			part_buf_start;
	u8			part_buf_count;
	union {
		u16		part_buf16;
		u32		part_buf32;
		u64		part_buf;
	};
	void (*push_data)(struct dw_mci *host, void *buf, int cnt);
	void (*pull_data)(struct dw_mci *host, void *buf, int cnt);

	/* Workaround flags */
	u32			quirks;

	/* S/W reset timer */
	struct timer_list       timer;
	bool			vqmmc_enabled;
	unsigned long		irq_flags; /* IRQ flags */
	int			irq;

	/* Save request status */
#define DW_MMC_REQ_IDLE		0
#define DW_MMC_REQ_BUSY		1
	unsigned int		req_state;
	struct dw_mci_debug_info        *debug_info;    /* debug info */

	/* HWACG q-active ctrl check */
	unsigned int qactive_check;

	/* Support system power mode */
	int idle_ip_index;

	/* For argos */
	unsigned int transferred_cnt;

	/* Sfr dump */
	struct dw_mci_sfr_ram_dump      *sfr_dump;

	/* S/W Timeout check */
	bool sw_timeout_chk;

	/* Card Clock In */
	u32			cclk_in;

	int			sdio_id0;

	struct timer_list       cmd11_timer;
	struct timer_list       dto_timer;
};

/* DMA ops for Internal/External DMAC interface */
struct dw_mci_dma_ops {
	/* DMA Ops */
	int (*init)(struct dw_mci *host);
	int (*start)(struct dw_mci *host, unsigned int sg_len);
	void (*complete)(void *host);
	void (*stop)(struct dw_mci *host);
	void (*reset)(struct dw_mci *host);
	void (*cleanup)(struct dw_mci *host);
	void (*exit)(struct dw_mci *host);
};

/* IP Quirks/flags. */
/* High Speed Capable - Supports HS cards (up to 50MHz) */
#define DW_MCI_QUIRK_HIGHSPEED                  BIT(0)
/* Unreliable card detection */
#define DW_MCI_QUIRK_BROKEN_CARD_DETECTION      BIT(1)
/* No write protect */
#define DW_MCI_QUIRK_NO_WRITE_PROTECT           BIT(2)
/* No detect end bit during read */
#define DW_MCI_QUIRK_NO_DETECT_EBIT             BIT(3)
/* Use fixed IO voltage */
#define DW_MMC_QUIRK_FIXED_VOLTAGE              BIT(4)
/* Card init W/A HWACG ctrl */
#define DW_MCI_QUIRK_HWACG_CTRL                 BIT(5)
/* Enables ultra low power mode */
#define DW_MCI_QUIRK_ENABLE_ULP                 BIT(6)
/* Use the security management unit */
#define DW_MCI_QUIRK_USE_SMU                    BIT(7)
/* Spread Spectrum Clock Cntrl */
#define DW_MCI_QUIRK_USE_SSC			BIT(8)
/* Timer for broken data transfer over scheme */
#define DW_MCI_QUIRK_BROKEN_DTO			BIT(9)

/* Slot level quirks */
/* This slot has no write protect */
#define DW_MCI_SLOT_QUIRK_NO_WRITE_PROTECT	BIT(0)
enum dw_mci_cd_types {
	DW_MCI_CD_INTERNAL = 1, /* use mmc internal CD line */
	DW_MCI_CD_EXTERNAL,     /* use external callback */
	DW_MCI_CD_GPIO,         /* use external gpio pin for CD line */
	DW_MCI_CD_NONE,         /* no CD line, use polling to detect card */
	DW_MCI_CD_PERMANENT,    /* no CD line, card permanently wired to host */
};
struct dma_pdata;

/* Board platform data */
struct dw_mci_board {
	u32 num_slots;

	u32 quirks; /* Workaround / Quirk flags */
	unsigned int bus_hz; /* Clock speed at the cclk_in pad */

	u32 caps;	/* Capabilities */
	u32 caps2;	/* More capabilities */
	u32 pm_caps;	/* PM capabilities */
	/*
	 * Override fifo depth. If 0, autodetect it from the FIFOTH register,
	 * but note that this may not be reliable after a bootloader has used
	 * it.
	 */
	unsigned int fifo_depth;

	/* delay in mS before detecting cards after interrupt */
	u32 detect_delay_ms;
	u8 clk_smpl;
	bool is_fine_tuned;
	bool tuned;
	bool extra_tuning;
	bool only_once_tune;

	/* INT QOS khz */
	unsigned int qos_dvfs_level;
	unsigned char io_mode;

	/* SSC RATE */
	unsigned int ssc_rate;

	enum dw_mci_cd_types cd_type;
	struct reset_control *rstc;
	struct dw_mci_dma_ops *dma_ops;
	struct dma_pdata *data;
	struct block_settings *blk_settings;
	unsigned int sw_timeout;

	/* DATA_TIMEOUT[31:11] of TMOUT */
	u32 data_timeout;
	u32 hto_timeout;
	bool use_gate_clock;
	bool use_biu_gate_clock;
	bool use_gpio_invert;
	bool enable_cclk_on_suspend;
	bool on_suspend;

	/* Number of descriptors */
	unsigned int desc_sz;
};

#ifdef CONFIG_MMC_DW_IDMAC
#define IDMAC_INT_CLR		(SDMMC_IDMAC_INT_AI | SDMMC_IDMAC_INT_NI | \
				 SDMMC_IDMAC_INT_CES | SDMMC_IDMAC_INT_DU | \
				 SDMMC_IDMAC_INT_FBE | SDMMC_IDMAC_INT_RI | \
				 SDMMC_IDMAC_INT_TI)

#if defined(CONFIG_MMC_DW_EXYNOS_FMP)
struct idmac_desc_64addr {
	u32		des0;	/* Control Descriptor */
#define IDMAC_DES0_DIC	BIT(1)
#define IDMAC_DES0_LD	BIT(2)
#define IDMAC_DES0_FD	BIT(3)
#define IDMAC_DES0_CH	BIT(4)
#define IDMAC_DES0_ER	BIT(5)
#define IDMAC_DES0_CES	BIT(30)
#define IDMAC_DES0_OWN	BIT(31)
	u32		des1;	/* Reserved */
#define IDMAC_64ADDR_SET_BUFFER1_SIZE(d, s) \
	((d)->des2 = ((d)->des2 & cpu_to_le32(0x03ffe000)) | \
	 ((cpu_to_le32(s)) & cpu_to_le32(0x1fff)))
	u32		des2;	/*Buffer sizes */
	u32		des3;	/* Reserved */
	u32		des4;	/* Lower 32-bits of Buffer Address Pointer 1*/
	u32		des5;	/* Upper 32-bits of Buffer Address Pointer 1*/
	u32		des6;	/* Lower 32-bits of Next Descriptor Address */
	u32		des7;	/* Upper 32-bits of Next Descriptor Address */
	u32		des8;	/* File IV 0 */
	u32		des9;	/* File IV 1 */
	u32		des10;	/* File IV 2 */
	u32		des11;	/* File IV 3 */
	u32		des12;	/* File EncKey 0 */
	u32		des13;	/* File EncKey 1 */
	u32		des14;	/* File EncKey 2 */
	u32		des15;	/* File EncKey 3 */
	u32		des16;	/* File EncKey 4 */
	u32		des17;	/* File EncKey 5 */
	u32		des18;	/* File EncKey 6 */
	u32		des19;	/* File EncKey 7 */
	u32		des20;	/* File TwKey 0 */
	u32		des21;	/* File TwKey 1 */
	u32		des22;	/* File TwKey 2 */
	u32		des23;	/* File TwKey 3 */
	u32		des24;	/* File TwKey 4 */
	u32		des25;	/* File TwKey 5 */
	u32		des26;	/* File TwKey 6 */
	u32		des27;	/* File TwKey 7 */
	u32		des28;	/* Disk IV 0 */
	u32		des29;	/* Disk IV 1 */
	u32		des30;	/* Disk IV 2 */
	u32		des31;	/* Disk IV 3 */
#if defined(CONFIG_EXYNOS_FMP_DUAL_FILE_ENCRYPTION)
	u32		des32;	/* File2 EncKey 0 */
	u32		des33;	/* File2 EncKey 1 */
	u32		des34;	/* File2 EncKey 2 */
	u32		des35;	/* File2 EncKey 3 */
	u32		des36;	/* File2 EncKey 4 */
	u32		des37;	/* File2 EncKey 5 */
	u32		des38;	/* File2 EncKey 6 */
	u32		des39;	/* File2 EncKey 7 */
	u32		des40;	/* File TwKey 0 */
	u32		des41;	/* File TwKey 1 */
	u32		des42;	/* File TwKey 2 */
	u32		des43;	/* File TwKey 3 */
	u32		des44;	/* File TwKey 4 */
	u32		des45;	/* File TwKey 5 */
	u32		des46;	/* File TwKey 6 */
	u32		des47;	/* File TwKey 7 */
	u32		des48;	/* Reserved */
	u32		des49;	/* Reserved */
	u32		des50;	/* Reserved */
	u32		des51;	/* Reserved */
	u32		des52;	/* Reserved */
	u32		des53;	/* Reserved */
	u32		des54;	/* Reserved */
	u32		des55;	/* Reserved */
	u32		des56;	/* Reserved */
	u32		des57;	/* Reserved */
	u32		des58;	/* Reserved */
	u32		des59;	/* Reserved */
	u32		des60;	/* Reserved */
	u32		des61;	/* Reserved */
	u32		des62;	/* Reserved */
	u32		des63;	/* Reserved */
#endif /* CONFIG_EXYNOS_FMP_DUAL_FILE_ENCRYPTION */
#define IDMAC_64ADDR_SET_DESC_CLEAR(d) \
do {			\
	(d)->des1 = 0;	\
	(d)->des2 = 0;	\
	(d)->des3 = 0;	\
} while(0)
#define IDMAC_64ADDR_SET_DESC_ADDR(d, a) \
do {			\
	(d)->des6 = ((u32)(a) & 0xffffffff); \
	(d)->des7 = ((u32)((a) >> 32));	\
} while(0)
};
#else
struct idmac_desc_64addr {
	u32		des0;	/* Control Descriptor */
#define IDMAC_OWN_CLR64(x) \
	!((x) & cpu_to_le32(IDMAC_DES0_OWN))

	u32		des1;	/* Reserved */

	u32		des2;	/*Buffer sizes */
#define IDMAC_64ADDR_SET_BUFFER1_SIZE(d, s) \
	((d)->des2 = ((d)->des2 & 0x03ffe000) | ((s) & 0x1fff))

	u32		des3;	/* Reserved */

	u32		des4;	/* Lower 32-bits of Buffer Address Pointer 1*/
	u32		des5;	/* Upper 32-bits of Buffer Address Pointer 1*/

	u32		des6;	/* Lower 32-bits of Next Descriptor Address */
	u32		des7;	/* Upper 32-bits of Next Descriptor Address */
#define IDMAC_64ADDR_SET_DESC_CLEAR(d) \
do {			\
	(d)->des1 = 0;	\
	(d)->des2 = 0;	\
	(d)->des3 = 0;	\
} while(0)
#define IDMAC_64ADDR_SET_DESC_ADDR(d, a) \
do {			\
	(d)->des6 = ((u32)(a) & 0xffffffff); \
	(d)->des7 = ((u32)((a) >> 32));	\
} while(0)
};
#endif

struct idmac_desc {
	u32		des0;	/* Control Descriptor */
#define IDMAC_DES0_DIC	BIT(1)
#define IDMAC_DES0_LD	BIT(2)
#define IDMAC_DES0_FD	BIT(3)
#define IDMAC_DES0_CH	BIT(4)
#define IDMAC_DES0_ER	BIT(5)
#define IDMAC_DES0_CES	BIT(30)
#define IDMAC_DES0_OWN	BIT(31)

	u32		des1;	/* Buffer sizes */
#define IDMAC_SET_BUFFER1_SIZE(d, s) \
	((d)->des1 = ((d)->des1 & 0x03ffe000) | ((s) & 0x1fff))

	u32		des2;	/* buffer 1 physical address */

	u32		des3;	/* buffer 2 physical address */
#define IDMAC_SET_DESC_ADDR(d, a) \
do {	\
	(d)->des3 = (u32)(a);	\
} while(0)
};
#endif /* CONFIG_MMC_DW_IDMAC */

/* FMP bypass/encrypt mode */
#define CLEAR		0
#define AES_CBC		1
#define AES_XTS		2

#endif /* LINUX_MMC_DW_MMC_H */
