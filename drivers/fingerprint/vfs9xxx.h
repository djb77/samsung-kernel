/*
 *  Copyright (C) 2017, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef VFS9XXX_H_
#define VFS9XXX_H_

#define VENDOR		"SYNAPTICS"
#define VALIDITY_PART_NAME "validity_fingerprint"

#define SLOW_BAUD_RATE     13000000
#define MAX_BAUD_RATE      13000000

#define BAUD_RATE_COEF      1000
#define DRDY_TIMEOUT_MS     40
#define DRDY_ACTIVE_STATUS  1
#define BITS_PER_WORD       8
#define DRDY_IRQ_FLAG       IRQF_TRIGGER_RISING
#define DEFAULT_BUFFER_SIZE (4096 * 16)
#define DRDY_IRQ_ENABLE			1
#define DRDY_IRQ_DISABLE		0
#define WAKEUP_ACTIVE_STATUS	0
#define WAKEUP_INACTIVE_STATUS	1
#define HBM_ON_STATUS			0
#define HBM_OFF_STATUS			1
#define FP_LDO_POWER_ON			1
#define FP_LDO_POWER_OFF		0

/* IOCTL commands definitions */

/* Magic number of IOCTL command */
#define VFSSPI_IOCTL_MAGIC    'k'

#ifndef ENABLE_SENSORS_FPRINT_SECURE
/* Transmit data to the device and retrieve data from it simultaneously */
#define VFSSPI_IOCTL_RW_SPI_MESSAGE         _IOWR(VFSSPI_IOCTL_MAGIC,	\
							 1, unsigned int)
#endif

/* Hard reset the device */
#define VFSSPI_IOCTL_DEVICE_RESET           _IO(VFSSPI_IOCTL_MAGIC,   2)
/* Set the baud rate of SPI master clock */
#define VFSSPI_IOCTL_SET_CLK                _IOW(VFSSPI_IOCTL_MAGIC,	\
							 3, unsigned int)
#ifndef ENABLE_SENSORS_FPRINT_SECURE
/* Get level state of DRDY GPIO */
#define VFSSPI_IOCTL_CHECK_DRDY             _IO(VFSSPI_IOCTL_MAGIC,   4)
#endif

/*
 * Register DRDY signal. It is used by SPI driver for indicating host that
 * DRDY signal is asserted.
 */
#define VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL   _IOW(VFSSPI_IOCTL_MAGIC,	\
							 5, unsigned int)

/* Enable/disable DRDY interrupt handling in the SPI driver */
#define VFSSPI_IOCTL_SET_DRDY_INT           _IOW(VFSSPI_IOCTL_MAGIC,	\
							8, unsigned int)
/* Put device in suspend state */
#define VFSSPI_IOCTL_DEVICE_SUSPEND         _IO(VFSSPI_IOCTL_MAGIC,   9)

#ifndef ENABLE_SENSORS_FPRINT_SECURE
/* Indicate the fingerprint buffer size for read */
#define VFSSPI_IOCTL_STREAM_READ_START      _IOW(VFSSPI_IOCTL_MAGIC,	\
						 10, unsigned int)
/* Indicate that fingerprint acquisition is completed */
#define VFSSPI_IOCTL_STREAM_READ_STOP       _IO(VFSSPI_IOCTL_MAGIC,   11)
#endif

/* Turn on the power to the sensor */
#define VFSSPI_IOCTL_POWER_ON               _IO(VFSSPI_IOCTL_MAGIC,   13)
/* Turn off the power to the sensor */
#define VFSSPI_IOCTL_POWER_OFF              _IO(VFSSPI_IOCTL_MAGIC,   14)

#ifdef ENABLE_SENSORS_FPRINT_SECURE
/* To disable spi core clock */
#define VFSSPI_IOCTL_DISABLE_SPI_CLOCK     _IO(VFSSPI_IOCTL_MAGIC,    15)
/* To set SPI configurations like gpio, clks */
#define VFSSPI_IOCTL_SET_SPI_CONFIGURATION _IO(VFSSPI_IOCTL_MAGIC,    16)
/* To reset SPI configurations */
#define VFSSPI_IOCTL_RESET_SPI_CONFIGURATION _IO(VFSSPI_IOCTL_MAGIC,  17)
/* To switch core */
#define VFSSPI_IOCTL_CPU_SPEEDUP     _IOW(VFSSPI_IOCTL_MAGIC,	\
						19, unsigned int)
#define VFSSPI_IOCTL_SET_SENSOR_TYPE     _IOW(VFSSPI_IOCTL_MAGIC,	\
							20, unsigned int)
#define VFSSPI_IOCTL_SET_LOCKSCREEN     _IOW(VFSSPI_IOCTL_MAGIC,	\
							22, unsigned int)
#endif

/* IOCTL #21 was already used Synaptics service. Do not use #21 */
#define VFSSPI_IOCTL_DUMMY     _IOWR(VFSSPI_IOCTL_MAGIC,	\
							21, unsigned int)

/* To control the power */
#define VFSSPI_IOCTL_POWER_CONTROL     _IOW(VFSSPI_IOCTL_MAGIC,	\
							23, unsigned int)
/* get sensor orienation from the SPI driver*/
#define VFSSPI_IOCTL_GET_SENSOR_ORIENT	\
	_IOR(VFSSPI_IOCTL_MAGIC, 18, unsigned int)

/*
 * Used by IOCTL command:
 *         VFSSPI_IOCTL_RW_SPI_MESSAGE
 *
 * @rx_buffer:pointer to retrieved data
 * @tx_buffer:pointer to transmitted data
 * @len:transmitted/retrieved data size
 */
#ifndef ENABLE_SENSORS_FPRINT_SECURE
struct vfsspi_ioctl_transfer {
	unsigned char *rx_buffer;
	unsigned char *tx_buffer;
	unsigned int len;
};
#endif

/*
 * Used by IOCTL command:
 *         VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL
 *
 * @user_pid:Process ID to which SPI driver sends signal indicating
 *			that DRDY is asserted
 * @signal_id:signal_id
 */
struct vfsspi_ioctl_register_signal {
	int user_pid;
	int signal_id;
};

/* CPID sensor event */
enum {
	CPID_EVENT_POWERDOWN = 1,
	CPID_EVENT_UNBLANK,
	CPID_EVENT_HBM_OFF,
	CPID_EVENT_HBM_ON,
};

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_mutex);
static struct class *vfsspi_device_class;
static int gpio_irq;

struct vfsspi_device_data {
	dev_t devt;
	struct cdev cdev;
	spinlock_t vfs_spi_lock;
	struct spi_device *spi;
	struct list_head device_entry;
	struct mutex buffer_mutex;
	unsigned int is_opened;
	unsigned char *buffer;
	unsigned char *null_buffer;
	unsigned char *stream_buffer;
	size_t stream_buffer_size;
	unsigned int drdy_pin;
	unsigned int sleep_pin;
	struct task_struct *t;
	int user_pid;
	int signal_id;
	unsigned int current_spi_speed;
	atomic_t irq_enabled;
	struct mutex kernel_lock;
	bool ldo_onoff;
	spinlock_t irq_lock;
	unsigned short drdy_irq_flag;
	unsigned int ldo_pin;
	const char *btp_vdd;
	struct regulator *regulator_3p3;
	int hbm_set;
	int hbm_pin;
	int wakeup_pin;
	int cnt_irq;
	const char *chipid;
	struct work_struct work_debug;
	struct workqueue_struct *wq_dbg;
	struct timer_list dbg_timer;
	struct pinctrl *p;
	struct pinctrl_state *pins_sleep;
	struct pinctrl_state *pins_idle;
	bool tz_mode;
	struct device *fp_device;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	bool enabled_clk;
	struct wake_lock fp_spi_lock;
#endif
	struct wake_lock fp_signal_lock;
	int sensortype;
	unsigned int orient;
	unsigned int detect_mode;
#ifdef CONFIG_FB
	struct notifier_block fb_notifier;
#endif
};

#ifdef ENABLE_SENSORS_FPRINT_SECURE
int fpsensor_goto_suspend;
#endif
static struct vfsspi_device_data *g_data;

#endif /* VFS9XXX_H_ */
