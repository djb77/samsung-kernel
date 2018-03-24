/*
 * Copyright (C) 2014 Samsung Electronics.
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

#ifndef __MODEM_IF_H__
#define __MODEM_IF_H__

#include <linux/platform_device.h>
#include <linux/miscdevice.h>

enum modem_t {
	SEC_CMC221,
	SEC_SS222,
	SEC_SH222AP,
	SEC_SH310AP,
	SEC_SH333AP,
	SEC_SH340AP,
	DUMMY,
	MAX_MODEM_TYPE
};

enum dev_format {
	IPC_FMT,
	IPC_RAW,
	IPC_RFS,
	IPC_MULTI_RAW,
	IPC_BOOT,
	IPC_DUMP,
	IPC_CMD,
	IPC_DEBUG,
	MAX_DEV_FORMAT,
};
#define MAX_IPC_DEV	(IPC_RFS + 1)	/* FMT, RAW, RFS */
#define MAX_SIPC5_DEV	(IPC_RAW + 1)	/* FMT, RAW */
#define MAX_EXYNOS_DEVICES (IPC_RAW + 1)	/* FMT, RAW */

enum modem_io {
	IODEV_MISC,
	IODEV_NET,
	IODEV_DUMMY,
};

enum modem_link {
	LINKDEV_UNDEFINED,
	LINKDEV_MIPI,
	LINKDEV_USB,
	LINKDEV_HSIC,
	LINKDEV_DPRAM,
	LINKDEV_PLD,
	LINKDEV_C2C,
	LINKDEV_SHMEM,
	LINKDEV_SPI,
	LINKDEV_MAX
};
#define LINKTYPE(modem_link) (1u << (modem_link))

enum ap_type {
	S5P,
	MAX_AP_TYPE
};

enum sipc_ver {
	NO_SIPC_VER = 0,
	SIPC_VER_40 = 40,
	SIPC_VER_41 = 41,
	SIPC_VER_42 = 42,
	SIPC_VER_50 = 50,
	MAX_SIPC_VER
};

#define STR_CP_FAIL "cp_fail"
#define STR_CP_WDT  "cp_wdt"    /* CP watchdog timer */

enum iodev_attr_bit {
	ATTR_SIPC4,
	ATTR_SIPC5,
	ATTR_CDC_NCM,
	ATTR_MULTIFMT,
	ATTR_HANDOVER,
	ATTR_LEGACY_RFS,
	ATTR_RX_FRAGMENT,
	ATTR_SBD_IPC,       /* IPC using SBD designed from MIPI-LLI */
	ATTR_NO_LINK_HEADER,    /* Link-layer header is not needed  */
	ATTR_NO_CHECK_MAXQ,     /* no need to check rxq overflow condition */
};
#define IODEV_ATTR(b)   (0x1 << b)

/**
 * struct modem_io_t - declaration for io_device
 * @name:	device name
 * @id:		for SIPC4, contains format & channel information
 *		(id & 11100000b)>>5 = format  (eg, 0=FMT, 1=RAW, 2=RFS)
 *		(id & 00011111b)    = channel (valid only if format is RAW)
 *		for SIPC5, contains only 8-bit channel ID
 * @format:	device format
 * @io_type:	type of this io_device
 * @links:	list of link_devices to use this io_device
 *		for example, if you want to use DPRAM and USB in an io_device.
 *		.links = LINKTYPE(LINKDEV_DPRAM) | LINKTYPE(LINKDEV_USB)
 * @tx_link:	when you use 2+ link_devices, set the link for TX.
 *		If define multiple link_devices in @links,
 *		you can receive data from them. But, cannot send data to all.
 *		TX is only one link_device.
 * @app:	the name of the application that will use this IO device
 *
 * This structure is used in board-*-modems.c
 */
struct modem_io_t {
	char *name;
	int   id;
	enum dev_format format;
	enum modem_io io_type;
	enum modem_link links;
	enum modem_link tx_link;
	u32 attrs;
	char *app;

	unsigned int ul_num_buffers;
	unsigned int ul_buffer_size;
	unsigned int dl_num_buffers;
	unsigned int dl_buffer_size;
};

struct modemlink_pm_data {
	char *name;
	/* link power contol 2 types : pin & regulator control */
	int (*link_ldo_enable)(bool);
	unsigned gpio_link_enable;
	unsigned gpio_link_active;
	unsigned gpio_link_hostwake;
	unsigned gpio_link_slavewake;
	int (*link_reconnect)(void);

	/* usb hub only */
	int (*port_enable)(int, int);
	int (*hub_standby)(void *);
	void *hub_pm_data;
	bool has_usbhub;

	/* cpu/bus frequency lock */
	atomic_t freqlock;
	int (*freq_lock)(struct device *dev);
	int (*freq_unlock)(struct device *dev);

	int autosuspend_delay_ms; /* if zero, the default value is used */
	void (*ehci_reg_dump)(struct device *);
};

struct modemlink_pm_link_activectl {
	int gpio_initialized;
	int gpio_request_host_active;
};

enum shmem_type {
	REAL_SHMEM,
	C2C_SHMEM,
	MAX_SHMEM_TYPE
};

#define STR_SHMEM_BASE		"shmem_base"

#define SHMEM_SIZE_1MB		(1 << 20)	/* 1 MB */
#define SHMEM_SIZE_2MB		(2 << 20)	/* 2 MB */
#define SHMEM_SIZE_4MB		(4 << 20)	/* 4 MB */

struct modem_mbox {
	unsigned mbx_ap2cp_msg;
	unsigned mbx_cp2ap_msg;
	unsigned mbx_ap2cp_active;	/* PDA_ACTIVE	*/
	unsigned mbx_cp2ap_active;	/* PHONE_ACTIVE	*/
	unsigned mbx_ap2cp_wakeup;	/* CP_WAKEUP	*/
	unsigned mbx_cp2ap_wakeup;	/* AP_WAKEUP	*/
	unsigned mbx_ap2cp_status;	/* AP_STATUS	*/
	unsigned mbx_cp2ap_status;	/* CP_STATUS	*/
	unsigned mbx_ap2cp_sec;		/* AP Time(sec)	*/
	unsigned mbx_ap2cp_usec;	/* AP Time(usec) */

	int int_ap2cp_msg;
	int int_ap2cp_active;
	int int_ap2cp_wakeup;
	int int_ap2cp_status;

	int irq_cp2ap_msg;
	int irq_cp2ap_active;
	int irq_cp2ap_wakeup;
	int irq_cp2ap_status;

	/* Performance request */
	unsigned int mbx_ap2cp_perf_req;
	unsigned int mbx_cp2ap_perf_req;
	unsigned int mbx_cp2ap_perf_req_cpu;
	unsigned int mbx_cp2ap_perf_req_mif;
	unsigned int mbx_cp2ap_perf_req_int;
	unsigned int mbx_cp2ap_pcie_l1ss_disable;

	unsigned int int_ap2cp_perf_req;
	unsigned int irq_cp2ap_perf_req;
	unsigned int irq_cp2ap_perf_req_cpu;
	unsigned int irq_cp2ap_perf_req_mif;
	unsigned int irq_cp2ap_perf_req_int;
	unsigned int irq_cp2ap_pcie_l1ss_disable;

	unsigned int mbx_ap2cp_et_dac_cal;

	/* System (H/W) revision */
	unsigned mbx_ap2cp_sys_rev;
	unsigned mbx_ap2cp_pmic_rev;
	unsigned mbx_ap2cp_pkg_id;
	unsigned mbx_ap2cp_lock_value;
};

struct modem_pmu {
	int (*power)(int);
	int (*init)(void);
	int (*get_pwr_status)(void);
	int (*stop)(void);
	int (*start)(void);
	int (*clear_cp_fail)(void);
	int (*clear_cp_wdt)(void);
};

/* platform data */
struct modem_data {
	char *name;

	unsigned gpio_cp_on;
	unsigned gpio_cp_off;
	unsigned gpio_reset_req_n;
	unsigned gpio_cp_reset;

	/* for broadcasting AP's PM state (active or sleep) */
	unsigned gpio_pda_active;

	/* for checking aliveness of CP */
	unsigned gpio_phone_active;
	int irq_phone_active;

	/* for AP-CP IPC interrupt */
	unsigned gpio_ipc_int2ap;
	int irq_ipc_int2ap;
	unsigned long irqf_ipc_int2ap;	/* IRQ flags */
	unsigned gpio_ipc_int2cp;

	/* for AP-CP power management (PM) handshaking */
	unsigned gpio_ap_wakeup;
	int irq_ap_wakeup;
	unsigned gpio_ap_status;
	unsigned gpio_cp_wakeup;
	unsigned gpio_cp_status;
	int irq_cp_status;

	/* for USB/HSIC PM */
	unsigned gpio_host_wakeup;
	int irq_host_wakeup;
	unsigned gpio_host_active;
	unsigned gpio_slave_wakeup;

	unsigned gpio_cp_dump_int;
	unsigned gpio_ap_dump_int;
	unsigned gpio_flm_uart_sel;
	unsigned gpio_cp_warm_reset;
#if defined(CONFIG_MACH_M0_CTC)
	unsigned gpio_flm_uart_sel_rev06;
#endif

	unsigned gpio_sim_detect;
	int irq_sim_detect;

#ifdef CONFIG_MACH_U1_KOR_LGT
	unsigned gpio_cp_reset_msm;
	unsigned gpio_boot_sw_sel;
	void (*vbus_on)(void);
	void (*vbus_off)(void);
	struct regulator *cp_vbus;
#endif

#ifdef CONFIG_TDSCDMA_MODEM_SPRD8803
	unsigned gpio_ipc_mrdy;
	unsigned gpio_ipc_srdy;
	unsigned gpio_ipc_sub_mrdy;
	unsigned gpio_ipc_sub_srdy;
	unsigned gpio_ap_cp_int1;
	unsigned gpio_ap_cp_int2;
#endif

#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	unsigned gpio_sim_io_sel;
	unsigned gpio_cp_ctrl1;
	unsigned gpio_cp_ctrl2;
#endif

#ifdef CONFIG_SOC_EXYNOS3470
	unsigned int hw_revision;
	unsigned int package_id;
	struct modem_mbox *mbx;
	struct modem_pmu *pmu;
#else
	unsigned int hw_revision;
	unsigned int package_id;
	unsigned int lock_value;
	struct modem_mbox *mbx;
	struct modem_pmu *pmu;

	int cp_active;
	int cp_wdt_reset;
#endif

	/* Switch with 2 links in a modem */
	unsigned gpio_link_switch;

	/* Modem component */
	enum modem_link link_types;
	char *link_name;

	/* the number of real IPC devices -> (IPC_RAW + 1) or (IPC_RFS + 1) */
	int max_ipc_dev;

	/* Information of IO devices */
	unsigned num_iodevs;
	struct modem_io_t *iodevs;

	/* Modem link PM support */
	struct modemlink_pm_data *link_pm_data;

	/* Handover with 2+ modems */
	bool use_handover;

	/* SIM Detect polarity */
	bool sim_polarity;

	/* SHDMEM ADDR */
	u32 shmem_base;
	u32 ipcmem_offset;
	u32 ipc_size;
	u32 dump_offset;
	u32 dump_addr;

	u8 __iomem *modem_base;
	u8 __iomem *dump_base;
	u8 __iomem *ipc_base;

	void (*gpio_revers_bias_clear)(void);
	void (*gpio_revers_bias_restore)(void);
};

#define MODEM_BOOT_DEV_SPI "modem_boot_spi"

struct modem_boot_spi_platform_data {
	const char *name;
	unsigned int gpio_cp_status;
};

struct modem_boot_spi {
	struct miscdevice dev;
	struct spi_device *spi_dev;
	struct mutex lock;
	unsigned gpio_cp_status;
};
#define to_modem_boot_spi(misc)	container_of(misc, struct modem_boot_spi, dev);

struct utc_time {
	u16 year;
	u8 mon:4,
	   day:4;
	u8 hour;
	u8 min;
	u8 sec;
	u16 msec;
	u16 us;
} __packed;

extern void get_utc_time(struct utc_time *utc);

#ifdef CONFIG_OF
#define mif_dt_read_enum(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) \
			return -EINVAL; \
		dest = (__typeof__(dest))(val); \
	} while (0)

#define mif_dt_read_bool(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) \
			return -EINVAL; \
		dest = val ? true : false; \
	} while (0)

#define mif_dt_read_string(np, prop, dest) \
	do { \
		if (of_property_read_string(np, prop, \
				(const char **)&dest)) \
		return -EINVAL; \
	} while (0)

#define mif_dt_read_u32(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) \
			return -EINVAL; \
				dest = val; \
	} while (0)
#endif

#define LOG_TAG	"mif: "
#define CALLEE	(__func__)
#define CALLER	(__builtin_return_address(0))

#define mif_err_limited(fmt, ...) \
	 printk_ratelimited(KERN_ERR "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_err(fmt, ...) \
	pr_err(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_debug(fmt, ...) \
	pr_debug(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_info(fmt, ...) \
	pr_info(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_trace(fmt, ...) \
	printk(KERN_DEBUG "mif: %s: %d: called(%pF): " fmt, \
		__func__, __LINE__, __builtin_return_address(0), ##__VA_ARGS__)

#endif
