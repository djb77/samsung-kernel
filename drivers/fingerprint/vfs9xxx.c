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

#include "fingerprint.h"

#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spi/spi.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/i2c/twl.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/irq.h>

#include <asm-generic/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <linux/pinctrl/consumer.h>
#include "../pinctrl/core.h"

#include <linux/platform_data/spi-s3c64xx.h>
#include <linux/regulator/consumer.h>
#include <linux/wakelock.h>

#ifdef ENABLE_SENSORS_FPRINT_SECURE
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/spi/spidev.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/amba/bus.h>
#include <linux/amba/pl330.h>
#if defined(CONFIG_SECURE_OS_BOOSTER_API)
#if defined(CONFIG_SOC_EXYNOS8890) || defined(CONFIG_SOC_EXYNOS8895)
#include <soc/samsung/secos_booster.h>
#else
#include <mach/secos_booster.h>
#endif
#endif
#include <linux/smc.h>
#include <linux/sysfs.h>

struct sec_spi_info {
	int port;
	unsigned long speed;
};

#endif

#include "vfs9xxx.h"

#ifdef CONFIG_FB
#include <linux/fb.h>
#endif

#ifdef CONFIG_OF
static const struct of_device_id vfsspi_match_table[] = {
	{.compatible = "vfsspi,vfs9xxx",},
	{},
};
#else
#define vfsspi_match_table NULL
#endif

#ifdef CONFIG_FB
static int vfsspi_callback_notifier(struct notifier_block *self,
				       unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank = evdata->data;

	if (!blank) {
		pr_err("%s: blank is null\n", __func__);
		return 0;
	}

	switch (*blank) {
	case FB_BLANK_UNBLANK:
		if (gpio_get_value(g_data->wakeup_pin) == WAKEUP_INACTIVE_STATUS) {
			pr_info("%s: FB_BLANK_UNBLANK\n", __func__);
			gpio_set_value(g_data->wakeup_pin, WAKEUP_ACTIVE_STATUS);
		}
		break;
	case FB_BLANK_POWERDOWN:
		if (gpio_get_value(g_data->wakeup_pin) == WAKEUP_ACTIVE_STATUS) {
			pr_info("%s: FB_BLANK_POWERDOWN\n", __func__);
			gpio_set_value(g_data->wakeup_pin, WAKEUP_INACTIVE_STATUS);
		}
		break;
	}

	return 0;
}
#endif

static int vfsspi_cpid_gpio_event(int event)
{
	int ret = 0;

	if (g_data != NULL) {
		switch (event) {
		case CPID_EVENT_POWERDOWN:
			pr_info("%s: EVENT_POWERDOWN\n", __func__);
			gpio_set_value(g_data->wakeup_pin,
					WAKEUP_INACTIVE_STATUS);
			break;
		case CPID_EVENT_UNBLANK:
			pr_info("%s: EVENT_UNBLANK\n", __func__);
			gpio_set_value(g_data->wakeup_pin,
					WAKEUP_ACTIVE_STATUS);
			break;
		case CPID_EVENT_HBM_OFF:
			pr_info("%s: EVENT_HBM_OFF\n", __func__);
			gpio_set_value(g_data->hbm_pin, HBM_OFF_STATUS);
			break;
		case CPID_EVENT_HBM_ON:
			pr_info("%s: EVENT_HBM_ON\n", __func__);
			gpio_set_value(g_data->hbm_pin, HBM_ON_STATUS);
			break;
		default:
			pr_err("%s: invalid event id\n", __func__);
			ret = -EINVAL;
			break;
		}
	}
	return ret;
}
EXPORT_SYMBOL(vfsspi_cpid_gpio_event);

static int vfsspi_send_drdy_signal(struct vfsspi_device_data *vfsspi_device)
{
	int ret = 0;

	pr_debug("%s\n", __func__);

	if (vfsspi_device->t) {
		/* notify DRDY signal to user process */
		ret = send_sig_info(vfsspi_device->signal_id,
				    (struct siginfo *)1, vfsspi_device->t);
		if (ret < 0)
			pr_err("%s Error sending signal\n", __func__);

	} else {
		pr_err("%s task_struct is not received yet\n", __func__);
	}

	return ret;
}

/* Return no. of bytes written to device. Negative number for errors */
static inline ssize_t vfsspi_write_sync(struct vfsspi_device_data *vfsspi_device,
				       size_t len)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));
	if (len != (unsigned int)len)
		pr_err
		    ("%s len casting is failed. t.len=%ld, len=%d",
		     __func__, len, (unsigned int)len);

	t.rx_buf = vfsspi_device->null_buffer;
	t.tx_buf = vfsspi_device->buffer;
	t.len = (unsigned int)len;
	t.speed_hz = vfsspi_device->current_spi_speed;

	spi_message_add_tail(&t, &m);

	status = spi_sync(vfsspi_device->spi, &m);

	if (status == 0)
		status = m.actual_length;

	return status;
}

/* Return no. of bytes read > 0. negative integer incase of error. */
static inline ssize_t vfsspi_read_sync(struct vfsspi_device_data *vfsspi_device,
				      size_t len)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;

	spi_message_init(&m);
	memset(&t, 0x0, sizeof(t));
	if (len != (unsigned int)len)
		pr_err
		    ("%s len casting is failed. t.len=%ld, len=%d",
		     __func__, len, (unsigned int)len);

	memset(vfsspi_device->null_buffer, 0x0, len);
	t.tx_buf = vfsspi_device->null_buffer;
	t.rx_buf = vfsspi_device->buffer;
	t.len = (unsigned int)len;
	t.speed_hz = vfsspi_device->current_spi_speed;

	spi_message_add_tail(&t, &m);

	status = spi_sync(vfsspi_device->spi, &m);

	if (status == 0)
		status = len;

	return status;
}

static ssize_t vfsspi_write(struct file *filp, const char __user *buf,
			    size_t count, loff_t *fPos)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	struct vfsspi_device_data *vfsspi_device = NULL;
	ssize_t status = 0;

	pr_debug("%s\n", __func__);

	if (count > DEFAULT_BUFFER_SIZE || !count)
		return -EMSGSIZE;

	vfsspi_device = filp->private_data;

	mutex_lock(&vfsspi_device->buffer_mutex);

	if (vfsspi_device->buffer) {
		unsigned long missing = 0;

		missing = copy_from_user(vfsspi_device->buffer, buf, count);

		if (missing == 0)
			status = vfsspi_write_sync(vfsspi_device, count);
		else
			status = -EFAULT;
	}

	mutex_unlock(&vfsspi_device->buffer_mutex);

	return status;
#endif
}

static ssize_t vfsspi_read(struct file *filp, char __user *buf,
			   size_t count, loff_t *fPos)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	struct vfsspi_device_data *vfsspi_device = NULL;
	ssize_t status = 0;

	pr_debug("%s\n", __func__);

	if (count > DEFAULT_BUFFER_SIZE || !count)
		return -EMSGSIZE;
	if (buf == NULL)
		return -EFAULT;

	vfsspi_device = filp->private_data;

	mutex_lock(&vfsspi_device->buffer_mutex);

	status = vfsspi_read_sync(vfsspi_device, count);

	if (status > 0) {
		unsigned long missing = 0;
		/* data read. Copy to user buffer. */
		missing = copy_to_user(buf, vfsspi_device->buffer, status);

		if (missing == status) {
			pr_err("%s copy_to_user failed\n", __func__);
			/* Nothing was copied to user space buffer. */
			status = -EFAULT;
		} else {
			status = status - missing;
		}
	}

	mutex_unlock(&vfsspi_device->buffer_mutex);

	return status;
#endif
}

#ifndef ENABLE_SENSORS_FPRINT_SECURE
static int vfsspi_xfer(struct vfsspi_device_data *vfsspi_device,
		       struct vfsspi_ioctl_transfer *tr)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;

	pr_debug("%s\n", __func__);

	if (vfsspi_device == NULL || tr == NULL)
		return -EFAULT;

	if (tr->len > DEFAULT_BUFFER_SIZE || !tr->len)
		return -EMSGSIZE;

	if (tr->tx_buffer != NULL) {
		if (copy_from_user
		    (vfsspi_device->null_buffer, tr->tx_buffer, tr->len) != 0)
			return -EFAULT;
	}

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	t.tx_buf = vfsspi_device->null_buffer;
	t.rx_buf = vfsspi_device->buffer;
	t.len = tr->len;
	t.speed_hz = vfsspi_device->current_spi_speed;

	spi_message_add_tail(&t, &m);

	status = spi_sync(vfsspi_device->spi, &m);

	if (status == 0) {
		if (tr->rx_buffer != NULL) {
			unsigned long missing = 0;

			missing = copy_to_user(tr->rx_buffer,
					       vfsspi_device->buffer, tr->len);

			if (missing != 0)
				tr->len = tr->len - missing;
		}
	}
	return status;
}				/* vfsspi_xfer */
#endif

#ifndef ENABLE_SENSORS_FPRINT_SECURE
static int vfsspi_rw_spi_message(struct vfsspi_device_data *vfsspi_device,
				 unsigned long arg)
{
	struct vfsspi_ioctl_transfer *dup = NULL;

	dup = kmalloc(sizeof(struct vfsspi_ioctl_transfer), GFP_KERNEL);
	if (dup == NULL)
		return -ENOMEM;

	if (copy_from_user(dup, (void *)arg,
			sizeof(struct vfsspi_ioctl_transfer)) == 0) {
		int err = vfsspi_xfer(vfsspi_device, dup);

		if (err != 0) {
			kfree(dup);
			return err;
		}
	} else {
		kfree(dup);
		return -EFAULT;
	}
	if (copy_to_user((void *)arg, dup,
			 sizeof(struct vfsspi_ioctl_transfer)) != 0) {
		kfree(dup);
		return -EFAULT;
	}
	kfree(dup);
	return 0;
}
#endif

#ifdef ENABLE_SENSORS_FPRINT_SECURE
static int vfsspi_prepare_sec_spi(struct sec_spi_info *spi_info,
				  struct spi_device *spi)
{
	struct clk *fp_spi_pclk, *fp_spi_sclk;

	fp_spi_pclk = clk_get(NULL, "fp-spi-pclk");

	if (IS_ERR(fp_spi_pclk)) {
		pr_err("Can't get fp_spi_pclk\n");
		return -1;
	}

	fp_spi_sclk = clk_get(NULL, "fp-spi-sclk");

	if (IS_ERR(fp_spi_sclk)) {
		pr_err("Can't get fp_spi_sclk\n");
		return -1;
	}

	clk_prepare_enable(fp_spi_pclk);
	clk_prepare_enable(fp_spi_sclk);
	clk_set_rate(fp_spi_sclk, spi_info->speed * 2);

	clk_put(fp_spi_pclk);
	clk_put(fp_spi_sclk);

	return 0;
}

static int vfsspi_unprepare_sec_spi(struct sec_spi_info *spi_info,
				    struct spi_device *spi)
{
	struct clk *fp_spi_pclk, *fp_spi_sclk;

	fp_spi_pclk = clk_get(NULL, "fp-spi-pclk");
	if (IS_ERR(fp_spi_pclk)) {
		pr_err("Can't get fp_spi_pclk\n");
		return -1;
	}

	fp_spi_sclk = clk_get(NULL, "fp-spi-sclk");

	if (IS_ERR(fp_spi_sclk)) {
		pr_err("Can't get fp_spi_sclk\n");
		return -1;
	}

	clk_disable_unprepare(fp_spi_pclk);
	clk_disable_unprepare(fp_spi_sclk);

	clk_put(fp_spi_pclk);
	clk_put(fp_spi_sclk);

	return 0;
}

#if !defined(CONFIG_SOC_EXYNOS8890) && !defined(CONFIG_SOC_EXYNOS8895)
struct amba_device *adev_dma;
static int vfsspi_prepare_sec_dma(struct sec_spi_info *spi_info)
{
	struct device_node *np;

	for_each_compatible_node(np, NULL, "arm,pl330") {
		if (!of_device_is_available(np))
			continue;

		if (!of_dma_secure_mode(np))
			continue;

		adev_dma = of_find_amba_device_by_node(np);
		pr_info("%s: device_name:%s\n", __func__,
			dev_name(&adev_dma->dev));
		break;
	}

	if (adev_dma == NULL)
		return -1;

	pm_runtime_get_sync(&adev_dma->dev);

	return 0;
}

static int vfsspi_unprepare_sec_dma(void)
{
	if (adev_dma == NULL)
		return -1;

	pm_runtime_put(&adev_dma->dev);

	return 0;
}
#endif
#endif

static void vfsspi_pin_control(struct vfsspi_device_data *vfsspi_device,
			       bool pin_set)
{
	int status = 0;

	vfsspi_device->p->state = NULL;
	if (pin_set) {
		if (!IS_ERR(vfsspi_device->pins_idle)) {
			status = pinctrl_select_state(vfsspi_device->p,
						      vfsspi_device->pins_idle);
			if (status)
				pr_err("%s: can't set pin default state\n",
				       __func__);
			pr_debug("%s idle\n", __func__);
		}
	} else {
		if (!IS_ERR(vfsspi_device->pins_sleep)) {
			status = pinctrl_select_state(vfsspi_device->p,
						      vfsspi_device->pins_sleep);
			if (status)
				pr_err("%s: can't set pin sleep state\n",
				       __func__);
			pr_debug("%s sleep\n", __func__);
		}
	}
}

static int vfsspi_set_clk(struct vfsspi_device_data *vfsspi_device,
			  unsigned long arg)
{
	int ret_val = 0;
	unsigned short clock = 0;
	struct spi_device *spidev = NULL;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	struct sec_spi_info *spi_info = NULL;
#endif

	if (copy_from_user(&clock, (void *)arg, sizeof(unsigned short)) != 0)
		return -EFAULT;

	spin_lock_irq(&vfsspi_device->vfs_spi_lock);
	spidev = spi_dev_get(vfsspi_device->spi);
	spin_unlock_irq(&vfsspi_device->vfs_spi_lock);
	if (spidev != NULL) {
		switch (clock) {
		case 0:	/* Running baud rate. */
			pr_debug("%s Running baud rate.\n", __func__);
			spidev->max_speed_hz = MAX_BAUD_RATE;
			vfsspi_device->current_spi_speed = MAX_BAUD_RATE;
			break;
		case 0xFFFF:	/* Slow baud rate */
			pr_debug("%s slow baud rate.\n", __func__);
			spidev->max_speed_hz = SLOW_BAUD_RATE;
			vfsspi_device->current_spi_speed = SLOW_BAUD_RATE;
			break;
		default:
			pr_debug("%s baud rate is %d.\n", __func__, clock);
#if defined(CONFIG_SOC_EXYNOS8890) || defined(CONFIG_SOC_EXYNOS8895)
			/* doesn't support to SPI clock rate 13MHz under */
			vfsspi_device->current_spi_speed = MAX_BAUD_RATE;
#else
			vfsspi_device->current_spi_speed = clock * BAUD_RATE_COEF;
#endif
			if (vfsspi_device->current_spi_speed > MAX_BAUD_RATE)
				vfsspi_device->current_spi_speed = MAX_BAUD_RATE;
			spidev->max_speed_hz = vfsspi_device->current_spi_speed;
			break;
		}

#ifdef ENABLE_SENSORS_FPRINT_SECURE
		if (!vfsspi_device->enabled_clk) {
			spi_info = kmalloc(sizeof(struct sec_spi_info),
					   GFP_KERNEL);
			if (spi_info != NULL) {
				spi_info->speed = spidev->max_speed_hz;
				ret_val =
				    vfsspi_prepare_sec_spi(spi_info, spidev);
				if (ret_val < 0)
					pr_err("%s: Unable to enable spi clk\n",
							__func__);
				else
					pr_info("%s ENABLE_SPI_CLOCK %ld\n",
							__func__, spi_info->speed);
#if !defined(CONFIG_SOC_EXYNOS8890) && !defined(CONFIG_SOC_EXYNOS8895)
				ret_val = vfsspi_prepare_sec_dma(spi_info);
				if (ret_val < 0)
					pr_err("%s: Unable to enable spi dma\n",
					       __func__);
#endif
				kfree(spi_info);
				wake_lock(&vfsspi_device->fp_spi_lock);
				vfsspi_device->enabled_clk = true;
			} else
				ret_val = -ENOMEM;
		}
#else
		pr_info("%s, clk speed: %d\n", __func__,
				vfsspi_device->current_spi_speed);
#endif
		spi_dev_put(spidev);
	}
	return ret_val;
}

#ifdef ENABLE_SENSORS_FPRINT_SECURE
static int vfsspi_disable_spi_clock(struct vfsspi_device_data
					  *vfsspi_device)
{
	struct spi_device *spidev = NULL;
	int ret_val = 0;
	struct sec_spi_info *spi_info = NULL;

	if (vfsspi_device->enabled_clk) {
		spin_lock_irq(&vfsspi_device->vfs_spi_lock);
		spidev = spi_dev_get(vfsspi_device->spi);
		spin_unlock_irq(&vfsspi_device->vfs_spi_lock);
		ret_val = vfsspi_unprepare_sec_spi(spi_info, spidev);
		if (ret_val < 0)
			pr_err("%s: couldn't disable spi clks\n", __func__);
#if !defined(CONFIG_SOC_EXYNOS8890) && !defined(CONFIG_SOC_EXYNOS8895)
		ret_val = vfsspi_unprepare_sec_dma();
		if (ret_val < 0)
			pr_err("%s: couldn't disable spi dma\n", __func__);
#endif
		spi_dev_put(spidev);
		wake_unlock(&vfsspi_device->fp_spi_lock);
		vfsspi_device->enabled_clk = false;
	}
	return ret_val;
}
#endif

static int vfsspi_register_drdy_signal(struct vfsspi_device_data *vfsspi_device,
				       unsigned long arg)
{
	struct vfsspi_ioctl_register_signal usr_signal;

	if (copy_from_user(&usr_signal, (void *)arg, sizeof(usr_signal)) == 0) {
		vfsspi_device->user_pid = usr_signal.user_pid;
		vfsspi_device->signal_id = usr_signal.signal_id;
		rcu_read_lock();
		/* find the task_struct associated with userpid */
		vfsspi_device->t =
		    pid_task(find_pid_ns(vfsspi_device->user_pid, &init_pid_ns),
			     PIDTYPE_PID);
		if (vfsspi_device->t == NULL) {
			pr_debug("%s No such pid\n", __func__);
			rcu_read_unlock();
			return -ENODEV;
		}
		rcu_read_unlock();
		pr_info("%s Searching task with PID=%08x, t = %p\n",
			__func__, vfsspi_device->user_pid, vfsspi_device->t);
	} else {
		pr_err("%s Failed copy from user.\n", __func__);
		return -EFAULT;
	}
	return 0;
}

static int vfsspi_enable_irq(struct vfsspi_device_data *vfsspi_device)
{
	pr_info("%s\n", __func__);
	spin_lock_irq(&vfsspi_device->irq_lock);
	if (atomic_read(&vfsspi_device->irq_enabled)
	    == DRDY_IRQ_ENABLE) {
		spin_unlock_irq(&vfsspi_device->irq_lock);
		pr_info("%s DRDY irq already enabled\n", __func__);
		return -EINVAL;
	}
	vfsspi_pin_control(vfsspi_device, true);
	enable_irq(gpio_irq);
	atomic_set(&vfsspi_device->irq_enabled, DRDY_IRQ_ENABLE);
	vfsspi_device->cnt_irq++;
	spin_unlock_irq(&vfsspi_device->irq_lock);
	return 0;
}

static int vfsspi_disable_irq(struct vfsspi_device_data *vfsspi_device)
{
	pr_info("%s\n", __func__);
	spin_lock_irq(&vfsspi_device->irq_lock);
	if (atomic_read(&vfsspi_device->irq_enabled) == DRDY_IRQ_DISABLE) {
		spin_unlock_irq(&vfsspi_device->irq_lock);
		pr_info("%s DRDY irq already disabled\n", __func__);
		return -EINVAL;
	}
	disable_irq_nosync(gpio_irq);
	atomic_set(&vfsspi_device->irq_enabled, DRDY_IRQ_DISABLE);
	vfsspi_pin_control(vfsspi_device, false);
	vfsspi_device->cnt_irq--;
	spin_unlock_irq(&vfsspi_device->irq_lock);
	return 0;
}

static irqreturn_t vfsspi_irq(int irq, void *context)
{
	struct vfsspi_device_data *vfsspi_device = context;

	/* Linux kernel is designed so that when you disable
	 * an edge-triggered interrupt, and the edge happens while
	 * the interrupt is disabled, the system will re-play the
	 * interrupt at enable time.
	 * Therefore, we are checking DRDY GPIO pin state to make sure
	 * if the interrupt handler has been called actually by DRDY
	 * interrupt and it's not a previous interrupt re-play
	 */

	if (gpio_get_value(vfsspi_device->drdy_pin) == DRDY_ACTIVE_STATUS) {
		spin_lock(&vfsspi_device->irq_lock);
		if (atomic_read(&vfsspi_device->irq_enabled) == DRDY_IRQ_ENABLE) {
			disable_irq_nosync(gpio_irq);
			atomic_set(&vfsspi_device->irq_enabled,
				   DRDY_IRQ_DISABLE);
			vfsspi_pin_control(vfsspi_device, false);
			vfsspi_device->cnt_irq--;
			spin_unlock(&vfsspi_device->irq_lock);
			vfsspi_send_drdy_signal(vfsspi_device);
			wake_lock_timeout(&vfsspi_device->fp_signal_lock, 3 * HZ);
			pr_info("%s disableIrq\n", __func__);
		} else {
			spin_unlock(&vfsspi_device->irq_lock);
			pr_info("%s irq already diabled\n", __func__);
		}
	}
	return IRQ_HANDLED;
}

static int vfsspi_set_drdy_int(struct vfsspi_device_data *vfsspi_device,
			       unsigned long arg)
{
	unsigned short drdy_enable_flag;

	if (copy_from_user(&drdy_enable_flag, (void *)arg,
			   sizeof(drdy_enable_flag)) != 0) {
		pr_err("%s Failed copy from user.\n", __func__);
		return -EFAULT;
	}
	if (drdy_enable_flag == 0)
		vfsspi_disable_irq(vfsspi_device);
	else {
		vfsspi_enable_irq(vfsspi_device);

		/* Workaround the issue where the system
		 * misses DRDY notification to host when
		 * DRDY pin was asserted before enabling
		 * device.
		 */

		if (gpio_get_value(vfsspi_device->drdy_pin) ==
		    DRDY_ACTIVE_STATUS) {
			pr_info("%s drdy pin is already active atatus\n",
				__func__);
			vfsspi_send_drdy_signal(vfsspi_device);
		}
	}
	return 0;
}

static void vfsspi_regulator_onoff(struct vfsspi_device_data *vfsspi_device,
				   bool onoff)
{
	if (vfsspi_device->ldo_pin) {
		if (onoff) {
			gpio_set_value(vfsspi_device->ldo_pin, 1);
			if (vfsspi_device->sleep_pin) {
				usleep_range(1000, 1050);
				gpio_set_value(vfsspi_device->sleep_pin, 1);
			}
		} else {
#ifdef ENABLE_SENSORS_FPRINT_SECURE
			pr_info("%s: cs_set smc ret = %d\n", __func__,
				exynos_smc(MC_FC_FP_CS_SET, 0, 0, 0));
#endif
			if (vfsspi_device->sleep_pin)
				gpio_set_value(vfsspi_device->sleep_pin, 0);
			gpio_set_value(vfsspi_device->ldo_pin, 0);
		}
		vfsspi_device->ldo_onoff = onoff;
		pr_info("%s:ldo %s\n", __func__, onoff ? "on" : "off");
	} else if (vfsspi_device->regulator_3p3) {
		int rc = 0;

		if (onoff) {
			rc = regulator_enable(vfsspi_device->regulator_3p3);
			if (rc) {
				pr_err
				    ("%s - btp ldo enable failed, rc=%d\n",
				     __func__, rc);
				goto done;
			}
			if (vfsspi_device->sleep_pin) {
				usleep_range(1000, 1050);
				gpio_set_value(vfsspi_device->sleep_pin, 1);
			}
		} else {
#ifdef ENABLE_SENSORS_FPRINT_SECURE
			pr_info("%s: cs_set smc ret = %d\n", __func__,
				exynos_smc(MC_FC_FP_CS_SET, 0, 0, 0));
#endif
			if (vfsspi_device->sleep_pin)
				gpio_set_value(vfsspi_device->sleep_pin, 0);
			rc = regulator_disable(vfsspi_device->regulator_3p3);
			if (rc) {
				pr_err
				    ("%s - btp ldo disable failed, rc=%d\n",
				     __func__, rc);
				goto done;
			}
		}
		vfsspi_device->ldo_onoff = onoff;
done:
		pr_info("%s:regulator %s\n", __func__,
			vfsspi_device->ldo_onoff ? "on" : "off");
	} else {
		pr_info("%s: can't control in this revion\n", __func__);
	}

}

static void vfsspi_hard_reset(struct vfsspi_device_data *vfsspi_device)
{
	pr_info("%s\n", __func__);

	if (vfsspi_device != NULL) {
		if (vfsspi_device->sleep_pin) {
			if (gpio_get_value(vfsspi_device->sleep_pin) == 1) {
				gpio_set_value(vfsspi_device->sleep_pin, 0);
				usleep_range(5000, 5050);
			}
			gpio_set_value(vfsspi_device->sleep_pin, 1);
			usleep_range(10000, 10050);
		}
	}
}

static void vfsspi_suspend(struct vfsspi_device_data *vfsspi_device)
{
	pr_info("%s\n", __func__);

	if (vfsspi_device != NULL) {
		if (vfsspi_device->sleep_pin) {
			spin_lock(&vfsspi_device->vfs_spi_lock);
			gpio_set_value(vfsspi_device->sleep_pin, 0);
			spin_unlock(&vfsspi_device->vfs_spi_lock);
		}
	}
}

static void vfsspi_power_on(struct vfsspi_device_data *vfsspi_device)
{
	if (vfsspi_device->ldo_onoff == FP_LDO_POWER_OFF)
		vfsspi_regulator_onoff(vfsspi_device, true);
	else
		pr_info("%s already on\n", __func__);
}

static void vfsspi_power_off(struct vfsspi_device_data *vfsspi_device)
{
	if (vfsspi_device->ldo_onoff == FP_LDO_POWER_ON)
		vfsspi_regulator_onoff(vfsspi_device, false);
	else
		pr_info("%s already off\n", __func__);
}

static long vfsspi_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret_val = 0;
	struct vfsspi_device_data *vfsspi_device = NULL;
	unsigned int onoff = 0;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	unsigned int type_check = -1;
	unsigned int lockscreen_mode = 0;
#endif

	if (_IOC_TYPE(cmd) != VFSSPI_IOCTL_MAGIC) {
		pr_err
		    ("%s invalid magic. cmd=0x%X Received=0x%X Expected=0x%X\n",
		     __func__, cmd, _IOC_TYPE(cmd), VFSSPI_IOCTL_MAGIC);
		return -ENOTTY;
	}

	vfsspi_device = filp->private_data;
	mutex_lock(&vfsspi_device->buffer_mutex);
	switch (cmd) {
	case VFSSPI_IOCTL_DEVICE_RESET:
		pr_debug("%s DEVICE_RESET\n", __func__);
		vfsspi_hard_reset(vfsspi_device);
		break;
	case VFSSPI_IOCTL_DEVICE_SUSPEND:
		pr_debug("%s DEVICE_SUSPEND\n", __func__);
		vfsspi_suspend(vfsspi_device);
		break;
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	case VFSSPI_IOCTL_RW_SPI_MESSAGE:
		pr_debug("%s RW_SPI_MESSAGE\n", __func__);
		ret_val = vfsspi_rw_spi_message(vfsspi_device, arg);
		if (ret_val)
			pr_err("%s RW_SPI_MESSAGE error %d\n",
			       __func__, ret_val);
		break;
#endif
	case VFSSPI_IOCTL_SET_CLK:
		pr_info("%s SET_CLK\n", __func__);
		ret_val = vfsspi_set_clk(vfsspi_device, arg);
		break;
	case VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL:
		pr_info("%s REGISTER_DRDY_SIGNAL\n", __func__);
		ret_val = vfsspi_register_drdy_signal(vfsspi_device, arg);
		break;
	case VFSSPI_IOCTL_SET_DRDY_INT:
		pr_info("%s SET_DRDY_INT\n", __func__);
		ret_val = vfsspi_set_drdy_int(vfsspi_device, arg);
		break;
	case VFSSPI_IOCTL_POWER_ON:
		pr_info("%s POWER_ON\n", __func__);
		vfsspi_power_on(vfsspi_device);
		break;
	case VFSSPI_IOCTL_POWER_OFF:
		pr_info("%s POWER_OFF\n", __func__);
		vfsspi_power_off(vfsspi_device);
		break;
	case VFSSPI_IOCTL_POWER_CONTROL:
		if (copy_from_user(&onoff, (void *)arg,
				sizeof(unsigned int)) != 0) {
			pr_err("%s Failed copy from user.(POWER_CONTROL)\n", __func__);
			mutex_unlock(&vfsspi_device->buffer_mutex);
			return -EFAULT;
		}
		pr_info("%s POWER_CONTROL %s\n", __func__, onoff?"on":"off");
		vfsspi_regulator_onoff(vfsspi_device, onoff);
		break;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	case VFSSPI_IOCTL_DISABLE_SPI_CLOCK:
		pr_info("%s DISABLE_SPI_CLOCK\n", __func__);
		ret_val = vfsspi_disable_spi_clock(vfsspi_device);
		break;

	case VFSSPI_IOCTL_SET_SPI_CONFIGURATION:
		break;
	case VFSSPI_IOCTL_RESET_SPI_CONFIGURATION:
		break;
	case VFSSPI_IOCTL_CPU_SPEEDUP:
		if (copy_from_user(&onoff, (void *)arg,
				   sizeof(unsigned int)) != 0) {
			pr_err("%s Failed copy from user.(CPU_SPEEDUP)\n",
			       __func__);
			mutex_unlock(&vfsspi_device->buffer_mutex);
			return -EFAULT;
		}
#if defined(CONFIG_SECURE_OS_BOOSTER_API)
		if (onoff) {
			u8 retry_cnt = 0;

			pr_info
			    ("%s CPU_SPEEDUP ON:%d\n",
			     __func__, onoff);
			do {
				ret_val = secos_booster_start(onoff - 1);
				retry_cnt++;
				if (ret_val) {
					pr_err
					    ("%s: booster start failed. (%d) retry: %d\n",
					     __func__, ret_val, retry_cnt);
					if (retry_cnt < 7)
						usleep_range(500, 510);
				}
			} while (ret_val && retry_cnt < 7);
		} else {
			pr_info("%s CPU_SPEEDUP OFF\n", __func__);
			ret_val = secos_booster_stop();
			if (ret_val)
				pr_err("%s: booster stop failed. (%d)\n",
				       __func__, ret_val);
		}
#else
		pr_info("%s CPU_SPEEDUP is not used\n", __func__);
#endif
		break;
	case VFSSPI_IOCTL_SET_SENSOR_TYPE:
		if (copy_from_user(&type_check, (void *)arg,
				   sizeof(unsigned int)) != 0) {
			pr_err("%s Failed copy from user.(SET_SENSOR_TYPE)\n",
			       __func__);
			mutex_unlock(&vfsspi_device->buffer_mutex);
			return -EFAULT;
		}
		if ((int)type_check >= SENSOR_OOO && (int)type_check < SENSOR_MAXIMUM) {
			if ((int)type_check == SENSOR_OOO && vfsspi_device->sensortype == SENSOR_FAILED) {
				pr_info("%s Maintain type check from out of oder :%s\n", __func__,
					sensor_status[g_data->sensortype + 2]);
			} else {
				vfsspi_device->sensortype = (int)type_check;
				pr_info("%s SET_SENSOR_TYPE :%s\n",
					__func__,
					sensor_status[g_data->sensortype + 2]);
			}
		} else {
			pr_err
			    ("%s SET_SENSOR_TYPE : invalid value %d\n",
			     __func__, (int)type_check);
			vfsspi_device->sensortype = SENSOR_UNKNOWN;
		}
		break;
	case VFSSPI_IOCTL_SET_LOCKSCREEN:
		if (copy_from_user(&lockscreen_mode,
				   (void *)arg, sizeof(unsigned int)) != 0) {
			pr_err
			    ("%s Failed copy from user.(SET_LOCKSCREEN_MODE)\n",
			     __func__);
			mutex_unlock(&vfsspi_device->buffer_mutex);
			return -EFAULT;
		}
		lockscreen_mode ? (fp_lockscreen_mode =
				   true) : (fp_lockscreen_mode = false);
		pr_info("%s SET_LOCKSCREEN :%s\n", __func__,
			fp_lockscreen_mode ? "ON" : "OFF");
		break;
#endif
	case VFSSPI_IOCTL_GET_SENSOR_ORIENT:
		pr_info("%s: orient is %d(0: normal, 1: upsidedown)\n",
			__func__, vfsspi_device->orient);
		if (copy_to_user((void *)arg,
				 &(vfsspi_device->orient),
				 sizeof(vfsspi_device->orient))
		    != 0) {
			ret_val = -EFAULT;
			pr_err("%s Failed copy to user.(GET_SENSOR_ORIENT)\n", __func__);
		}
		break;
	case VFSSPI_IOCTL_DUMMY:
		break;
	default:
		pr_info("%s default error. %u\n", __func__, cmd);
		ret_val = -EFAULT;
		break;
	}
	mutex_unlock(&vfsspi_device->buffer_mutex);
	return ret_val;
}

static int vfsspi_open(struct inode *inode, struct file *filp)
{
	struct vfsspi_device_data *vfsspi_device = NULL;
	int status = -ENXIO;

	pr_info("%s\n", __func__);

	mutex_lock(&device_list_mutex);

	list_for_each_entry(vfsspi_device, &device_list, device_entry) {
		if (vfsspi_device->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if (status == 0) {
		mutex_lock(&vfsspi_device->kernel_lock);
		if (vfsspi_device->is_opened != 0) {
			status = -EBUSY;
			pr_err("%s is_opened != 0, -EBUSY\n", __func__);
			goto vfsspi_open_out;
		}
		vfsspi_device->user_pid = 0;
		if (vfsspi_device->buffer != NULL) {
			pr_err("%s buffer != NULL\n", __func__);
			goto vfsspi_open_out;
		}
		vfsspi_device->null_buffer =
		    kmalloc(DEFAULT_BUFFER_SIZE, GFP_KERNEL);
		if (vfsspi_device->null_buffer == NULL) {
			status = -ENOMEM;
			pr_err("%s null_buffer == NULL, -ENOMEM\n",
			       __func__);
			goto vfsspi_open_out;
		}
		vfsspi_device->buffer =
		    kmalloc(DEFAULT_BUFFER_SIZE, GFP_KERNEL);
		if (vfsspi_device->buffer == NULL) {
			status = -ENOMEM;
			kfree(vfsspi_device->null_buffer);
			pr_err("%s buffer == NULL, -ENOMEM\n",
			       __func__);
			goto vfsspi_open_out;
		}
		vfsspi_device->is_opened = 1;
		filp->private_data = vfsspi_device;
		nonseekable_open(inode, filp);

vfsspi_open_out:
		mutex_unlock(&vfsspi_device->kernel_lock);
	}
	mutex_unlock(&device_list_mutex);
	return status;
}

static int vfsspi_release(struct inode *inode, struct file *filp)
{
	struct vfsspi_device_data *vfsspi_device = NULL;
	int status = 0;

	pr_info("%s\n", __func__);

	mutex_lock(&device_list_mutex);
	vfsspi_device = filp->private_data;
	filp->private_data = NULL;
	vfsspi_device->is_opened = 0;
	if (vfsspi_device->buffer != NULL) {
		kfree(vfsspi_device->buffer);
		vfsspi_device->buffer = NULL;
	}

	if (vfsspi_device->null_buffer != NULL) {
		kfree(vfsspi_device->null_buffer);
		vfsspi_device->null_buffer = NULL;
	}

	mutex_unlock(&device_list_mutex);
	return status;
}

/* file operations associated with device */
static const struct file_operations vfsspi_fops = {
	.owner = THIS_MODULE,
	.write = vfsspi_write,
	.read = vfsspi_read,
	.unlocked_ioctl = vfsspi_ioctl,
	.open = vfsspi_open,
	.release = vfsspi_release,
};

static int vfsspi_init_platform(struct vfsspi_device_data *vfsspi_device)
{
	int status = 0;

	pr_info("%s\n", __func__);
	if (gpio_request(vfsspi_device->drdy_pin, "vfsspi_drdy") < 0) {
		status = -EBUSY;
		goto vfsspi_init_platform_drdy_failed;
	} else {
		status = gpio_direction_input(vfsspi_device->drdy_pin);
		if (status < 0) {
			pr_err("%s gpio_direction_input DRDY failed\n",
			       __func__);
			status = -EBUSY;
			goto vfsspi_init_platform_gpio_init_failed;
		}
	}
	if (vfsspi_device->sleep_pin) {
		if (gpio_request(vfsspi_device->sleep_pin, "vfsspi_sleep")) {
			status = -EBUSY;
			goto vfsspi_init_platform_gpio_init_failed;
		}
		status = gpio_direction_output(vfsspi_device->sleep_pin, 1);
		if (status < 0) {
			pr_err("%s gpio_direction_output SLEEP failed\n",
			       __func__);
			status = -EBUSY;
			goto vfsspi_init_platform_sleep_failed;
		}
	}
	if (vfsspi_device->ldo_pin) {
		status = gpio_request(vfsspi_device->ldo_pin, "vfsspi_ldo_en");
		if (status < 0) {
			pr_err("%s gpio_request vfsspi_ldo_en failed\n",
			       __func__);
			goto vfsspi_init_platform_sleep_failed;
		}
		status = gpio_direction_output(vfsspi_device->ldo_pin, 0);
		if (status < 0) {
			pr_err("%s gpio_direction_output ldo failed\n",
			       __func__);
			status = -EBUSY;
			goto vfsspi_init_platform_ldo_failed;
		}
	}

	if (gpio_request(vfsspi_device->hbm_pin, "vfsspi_hbm")) {
		status = -EBUSY;
		goto vfsspi_init_platform_ldo_failed;
	} else {
		status = gpio_direction_output(vfsspi_device->hbm_pin,
						HBM_OFF_STATUS);
		if (status < 0) {
			pr_err("%s gpio_direction_output hbm_pin failed\n",
			       __func__);
			status = -EBUSY;
			goto vfsspi_init_platform_hbm_failed;
		}
	}

	if (gpio_request(vfsspi_device->wakeup_pin, "vfsspi_wakeup")) {
		status = -EBUSY;
		goto vfsspi_init_platform_hbm_failed;
	} else {
		status = gpio_direction_output(vfsspi_device->wakeup_pin,
				WAKEUP_ACTIVE_STATUS);
		if (status < 0) {
			pr_err("%s gpio_direction_output wakeup_pin failed\n",
			       __func__);
			status = -EBUSY;
			goto vfsspi_init_platform_wakeup_failed;
		}
	}
	vfsspi_cpid_gpio_event(CPID_EVENT_UNBLANK);
	vfsspi_cpid_gpio_event(CPID_EVENT_HBM_OFF);

	spin_lock_init(&vfsspi_device->irq_lock);

	gpio_irq = gpio_to_irq(vfsspi_device->drdy_pin);

	if (gpio_irq < 0) {
		pr_err("%s gpio_to_irq failed\n", __func__);
		status = -EBUSY;
		goto vfsspi_init_platform_wakeup_failed;
	}

	if (request_irq(gpio_irq, vfsspi_irq, IRQF_TRIGGER_RISING,
			"vfsspi_irq", vfsspi_device) < 0) {
		pr_err("%s request_irq failed\n", __func__);
		status = -EBUSY;
		goto vfsspi_init_platform_irq_failed;
	}
	disable_irq(gpio_irq);
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	wake_lock_init(&vfsspi_device->fp_spi_lock,
				WAKE_LOCK_SUSPEND, "vfsspi_wake_lock");
#endif
	wake_lock_init(&vfsspi_device->fp_signal_lock,
				WAKE_LOCK_SUSPEND, "vfsspi_sigwake_lock");

	pr_info("%s : platformInit success!\n", __func__);
	return status;

vfsspi_init_platform_irq_failed:
vfsspi_init_platform_wakeup_failed:
	gpio_free(vfsspi_device->wakeup_pin);
vfsspi_init_platform_hbm_failed:
	gpio_free(vfsspi_device->hbm_pin);
vfsspi_init_platform_ldo_failed:
	if (vfsspi_device->ldo_pin)
		gpio_free(vfsspi_device->ldo_pin);
	if (vfsspi_device->regulator_3p3)
		regulator_put(vfsspi_device->regulator_3p3);
vfsspi_init_platform_sleep_failed:
	if (vfsspi_device->sleep_pin)
		gpio_free(vfsspi_device->sleep_pin);
vfsspi_init_platform_gpio_init_failed:
	gpio_free(vfsspi_device->drdy_pin);
vfsspi_init_platform_drdy_failed:
	pr_err("%s : platformInit failed!\n", __func__);
	return status;
}

static void vfsspi_uninit_platform(struct vfsspi_device_data *vfsspi_device)
{
	pr_info("%s\n", __func__);

	if (vfsspi_device != NULL) {
		free_irq(gpio_irq, vfsspi_device);
		vfsspi_device->drdy_irq_flag = DRDY_IRQ_DISABLE;
		if (vfsspi_device->sleep_pin)
			gpio_free(vfsspi_device->sleep_pin);
		if (vfsspi_device->drdy_pin)
			gpio_free(vfsspi_device->drdy_pin);
		if (vfsspi_device->ldo_pin)
			gpio_free(vfsspi_device->ldo_pin);
		if (vfsspi_device->regulator_3p3)
			regulator_put(vfsspi_device->regulator_3p3);
		gpio_free(vfsspi_device->hbm_pin);
		gpio_free(vfsspi_device->wakeup_pin);
#ifdef ENABLE_SENSORS_FPRINT_SECURE
		wake_lock_destroy(&vfsspi_device->fp_spi_lock);
#endif
		wake_lock_destroy(&vfsspi_device->fp_signal_lock);
	}
}

static int vfsspi_parse_dt(struct device *dev, struct vfsspi_device_data *data)
{
	struct device_node *np = dev->of_node;
	int errorno = 0;
	int gpio;

	gpio = of_get_named_gpio(np, "vfsspi-sleepPin", 0);
	if (gpio < 0) {
		errorno = gpio;
		pr_err("%s: fail to get sleep_pin\n", __func__);
		goto dt_exit;
	} else {
		data->sleep_pin = gpio;
		pr_info("%s: sleepPin=%d\n", __func__, data->sleep_pin);
	}

	gpio = of_get_named_gpio(np, "vfsspi-drdyPin", 0);
	if (gpio < 0) {
		errorno = gpio;
		pr_err("%s: fail to get drdy_pin\n", __func__);
		goto dt_exit;
	} else {
		data->drdy_pin = gpio;
		pr_info("%s: drdyPin=%d\n", __func__, data->drdy_pin);
	}

	gpio = of_get_named_gpio(np, "vfsspi-ldoPin", 0);
	if (gpio < 0) {
		data->ldo_pin = 0;
		pr_info("%s: not use ldo_pin\n", __func__);
	} else {
		data->ldo_pin = gpio;
		pr_info("%s: ldo_pin=%d\n", __func__, data->ldo_pin);
	}

	if (of_property_read_string(np, "vfsspi-regulator", &data->btp_vdd) < 0) {
		pr_info("%s: not use btp_regulator\n", __func__);
		data->btp_vdd = 0;
	} else {
		data->regulator_3p3 = regulator_get(NULL, data->btp_vdd);
		if (IS_ERR(data->regulator_3p3) ||
				(data->regulator_3p3) == NULL) {
			pr_info("%s: not use regulator_3p3\n", __func__);
			data->regulator_3p3 = 0;
		} else {
			pr_info("%s: vfsspi_regulator ok\n", __func__);
		}
	}

	gpio = of_get_named_gpio(np, "vfsspi-hbmPin", 0);
	if (gpio < 0) {
		errorno = gpio;
		pr_err("%s: fail to get hbm_pin\n", __func__);
		goto dt_exit;
	} else {
		data->hbm_pin = gpio;
		pr_info("%s: hbmPin=%d\n", __func__, data->hbm_pin);
	}

	gpio = of_get_named_gpio(np, "vfsspi-wakeupPin", 0);
	if (gpio < 0) {
		errorno = gpio;
		pr_err("%s: fail to get wakeup_pin\n", __func__);
		goto dt_exit;
	} else {
		data->wakeup_pin = gpio;
		pr_info("%s: wakeupPin=%d\n", __func__, data->wakeup_pin);
	}

	if (of_property_read_string_index(np, "vfsspi-chipid", 0,
					  (const char **)&data->chipid)) {
		data->chipid = NULL;
	}
	pr_info("%s: chipid: %s\n", __func__, data->chipid);

	if (of_property_read_u32(np, "vfsspi-wog", &data->detect_mode))
		data->detect_mode = DETECT_ADM;
	pr_info("%s: wog: %d\n", __func__, data->detect_mode);

#ifdef ENABLE_SENSORS_FPRINT_SECURE
	data->tz_mode = true;
#endif

	data->p = pinctrl_get_select_default(dev);
	if (IS_ERR(data->p)) {
		errorno = -EINVAL;
		pr_err("%s: failed pinctrl_get\n", __func__);
		goto dt_exit;
	}

	data->pins_sleep = pinctrl_lookup_state(data->p, PINCTRL_STATE_SLEEP);
	if (IS_ERR(data->pins_sleep)) {
		errorno = -EINVAL;
		pr_err("%s : could not get pins sleep_state (%li)\n",
		       __func__, PTR_ERR(data->pins_sleep));
		goto fail_pinctrl_get;
	}

	data->pins_idle = pinctrl_lookup_state(data->p, PINCTRL_STATE_IDLE);
	if (IS_ERR(data->pins_idle)) {
		errorno = -EINVAL;
		pr_err("%s : could not get pins idle_state (%li)\n",
		       __func__, PTR_ERR(data->pins_idle));
		goto fail_pinctrl_get;
	}
	return 0;
fail_pinctrl_get:
	pinctrl_put(data->p);
dt_exit:
	return errorno;
}

static ssize_t vfsspi_bfs_values_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct vfsspi_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "\"FP_SPICLK\":\"%d\"\n",
			data->current_spi_speed);
}

static ssize_t vfsspi_type_check_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct vfsspi_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->sensortype);
}

static ssize_t vfsspi_vendor_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t vfsspi_name_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", g_data->chipid);
}

static ssize_t vfsspi_adm_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", g_data->detect_mode);
}

static ssize_t vfsspi_hbm_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", g_data->hbm_set);
}

static ssize_t vfsspi_hbm_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct vfsspi_device_data *data = dev_get_drvdata(dev);
	int64_t enable;
	int ret = 0;

	ret = kstrtoll(buf, 10, &enable);
	if (ret < 0) {
		pr_err("%s data convert failed\n", __func__);
		return size;
	}

	if (enable == 1) {
		pr_info("%s hbm on %d\n", __func__, (int)enable);
		gpio_set_value(data->hbm_pin, HBM_ON_STATUS);
		data->hbm_set = (int)enable;
	} else if (enable == 0) {
		pr_info("%s hbm off %d\n", __func__, (int)enable);
		gpio_set_value(data->hbm_pin, HBM_OFF_STATUS);
		data->hbm_set = (int)enable;
	} else {
		pr_err("%s out of bound %d\n", __func__, (int)enable);
		return size;
	}
	return size;
}

static DEVICE_ATTR(bfs_values, 0444, vfsspi_bfs_values_show, NULL);
static DEVICE_ATTR(type_check, 0444, vfsspi_type_check_show, NULL);
static DEVICE_ATTR(vendor, 0444, vfsspi_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, vfsspi_name_show, NULL);
static DEVICE_ATTR(adm, 0444, vfsspi_adm_show, NULL);
static DEVICE_ATTR(hbm, 0664, vfsspi_hbm_show, vfsspi_hbm_store);

static struct device_attribute *fp_attrs[] = {
	&dev_attr_bfs_values,
	&dev_attr_type_check,
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_adm,
	&dev_attr_hbm,
	NULL,
};

static void vfsspi_work_func_debug(struct work_struct *work)
{
	pr_info("%s:CPID power:%d, irq:%d, tz:%d, type:%s\n",
		__func__,
		g_data->ldo_onoff,
		gpio_get_value(g_data->drdy_pin),
		g_data->tz_mode,
		sensor_status[g_data->sensortype + 2]);
}

static void vfsspi_enable_debug_timer(void)
{
	mod_timer(&g_data->dbg_timer,
		  round_jiffies_up(jiffies + FPSENSOR_DEBUG_TIMER_SEC));
}

static void vfsspi_disable_debug_timer(void)
{
	del_timer_sync(&g_data->dbg_timer);
	cancel_work_sync(&g_data->work_debug);
}

static void vfsspi_timer_func(unsigned long ptr)
{
	queue_work(g_data->wq_dbg, &g_data->work_debug);
	mod_timer(&g_data->dbg_timer,
		  round_jiffies_up(jiffies + FPSENSOR_DEBUG_TIMER_SEC));
}

#ifndef ENABLE_SENSORS_FPRINT_SECURE
static int vfsspi_type_check(struct vfsspi_device_data *vfsspi_device)
{
	struct spi_device *spi = NULL;
	int i = 0, retry = 0;

	pr_info("%s\n", __func__);
	vfsspi_power_on(vfsspi_device);
	vfsspi_hard_reset(vfsspi_device);
	mdelay(25);
	spi = vfsspi_device->spi;
	spi->bits_per_word = BITS_PER_WORD;
	spi->mode = SPI_MODE_0;
	spi_setup(spi);
	do {
		char tx_buf[64] = {0xa2, 0x01, 0xb5, 0x36, 0x5d, 0xfc, 0,};
		char rx_buf[64] = {0};
		struct spi_transfer t;
		struct spi_message m;

		memset(&t, 0, sizeof(t));
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 6;
		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		spi_sync(spi, &m);

		usleep_range(10000, 10050);
		memset(&tx_buf, 0, sizeof(tx_buf));
		tx_buf[0] = 0x23;
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 44;
		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		spi_sync(spi, &m);
		if (rx_buf[15] == 0x3A) {
			vfsspi_device->sensortype = SENSOR_CPID;
			pr_info("%s sensor type is CPID\n", __func__);
		} else {
			vfsspi_device->sensortype = SENSOR_FAILED;
			pr_info("%s sensor type is not CPID\n", __func__);
			for (i = 0; i < 16; i++)
				pr_info("%s, %0x\n", __func__, rx_buf[i]);
		}
	} while (!vfsspi_device->sensortype && (++retry < 3));
	vfsspi_power_off(vfsspi_device);
	return 0;
}
#endif

static int vfsspi_probe(struct spi_device *spi)
{
	int status = 0;
	struct vfsspi_device_data *vfsspi_device;
	struct device *dev;

	pr_info("%s: CPID\n", __func__);

	vfsspi_device = kzalloc(sizeof(*vfsspi_device), GFP_KERNEL);

	if (vfsspi_device == NULL)
		return -ENOMEM;

	if (spi->dev.of_node) {
		status = vfsspi_parse_dt(&spi->dev, vfsspi_device);
		if (status) {
			pr_err("%s - Failed to parse DT\n", __func__);
			goto vfsspi_probe_parse_dt_failed;
		}
	}

	/* Initialize driver data. */
	vfsspi_device->current_spi_speed = SLOW_BAUD_RATE;
	vfsspi_device->spi = spi;
	g_data = vfsspi_device;

	spin_lock_init(&vfsspi_device->vfs_spi_lock);
	mutex_init(&vfsspi_device->buffer_mutex);
	mutex_init(&vfsspi_device->kernel_lock);

	INIT_LIST_HEAD(&vfsspi_device->device_entry);

	status = vfsspi_init_platform(vfsspi_device);
	if (status) {
		pr_err("%s - Failed to platformInit\n", __func__);
		goto vfsspi_probe_platformInit_failed;
	}

	spi->bits_per_word = BITS_PER_WORD;
	spi->max_speed_hz = SLOW_BAUD_RATE;
	spi->mode = SPI_MODE_0;
	vfsspi_device->cnt_irq = 0;
	fp_lockscreen_mode = false;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	fpsensor_goto_suspend = 0;
#endif
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	status = spi_setup(spi);
	if (status) {
		pr_err("%s : spi_setup failed\n", __func__);
		goto vfsspi_probe_spi_setup_failed;
	}
#endif

	mutex_lock(&device_list_mutex);
	/* Create device node */
	/* register major number for character device */
	status = alloc_chrdev_region(&(vfsspi_device->devt),
				     0, 1, VALIDITY_PART_NAME);
	if (status < 0) {
		pr_err("%s alloc_chrdev_region failed\n", __func__);
		goto vfsspi_probe_alloc_chardev_failed;
	}

	cdev_init(&(vfsspi_device->cdev), &vfsspi_fops);
	vfsspi_device->cdev.owner = THIS_MODULE;
	status = cdev_add(&(vfsspi_device->cdev), vfsspi_device->devt, 1);
	if (status < 0) {
		pr_err("%s cdev_add failed\n", __func__);
		goto vfsspi_probe_cdev_add_failed;
	}

	vfsspi_device_class = class_create(THIS_MODULE, "validity_fingerprint");

	if (IS_ERR(vfsspi_device_class)) {
		pr_err
		    ("%s class_create() is failed\n",
		     __func__);
		status = PTR_ERR(vfsspi_device_class);
		goto vfsspi_probe_class_create_failed;
	}

	dev = device_create(vfsspi_device_class, &spi->dev,
			    vfsspi_device->devt, vfsspi_device, "vfsspi");
	status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	if (status == 0)
		list_add(&vfsspi_device->device_entry, &device_list);
	mutex_unlock(&device_list_mutex);

	if (status != 0)
		goto vfsspi_probe_failed;

	spi_set_drvdata(spi, vfsspi_device);

	status = fingerprint_register(vfsspi_device->fp_device,
				      vfsspi_device, fp_attrs, "fingerprint");
	if (status) {
		pr_err("%s sysfs register failed\n", __func__);
		goto vfsspi_probe_failed;
	}

	/* debug polling function */
	setup_timer(&vfsspi_device->dbg_timer,
		    vfsspi_timer_func, (unsigned long)vfsspi_device);

	vfsspi_device->wq_dbg =
	    create_singlethread_workqueue("vfsspi_debug_wq");
	if (!vfsspi_device->wq_dbg) {
		status = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto vfsspi_sysfs_failed;
	}
	INIT_WORK(&vfsspi_device->work_debug, vfsspi_work_func_debug);

#ifdef ENABLE_SENSORS_FPRINT_SECURE
	vfsspi_device->sensortype = SENSOR_UNKNOWN;
#else
	/* sensor hw type check */
	vfsspi_type_check(vfsspi_device);
#endif

	vfsspi_pin_control(vfsspi_device, false);
	vfsspi_enable_debug_timer();
#ifdef CONFIG_FB
	vfsspi_device->fb_notifier.notifier_call = vfsspi_callback_notifier;
	fb_register_client(&vfsspi_device->fb_notifier);
#endif
	pr_info("%s successful\n", __func__);

	return 0;

vfsspi_sysfs_failed:
	fingerprint_unregister(vfsspi_device->fp_device, fp_attrs);
vfsspi_probe_failed:
	device_destroy(vfsspi_device_class, vfsspi_device->devt);
	class_destroy(vfsspi_device_class);
vfsspi_probe_class_create_failed:
	cdev_del(&(vfsspi_device->cdev));
vfsspi_probe_cdev_add_failed:
	unregister_chrdev_region(vfsspi_device->devt, 1);
vfsspi_probe_alloc_chardev_failed:
#ifndef ENABLE_SENSORS_FPRINT_SECURE
vfsspi_probe_spi_setup_failed:
#endif
	vfsspi_uninit_platform(vfsspi_device);
vfsspi_probe_platformInit_failed:
	pinctrl_put(vfsspi_device->p);
	mutex_destroy(&vfsspi_device->buffer_mutex);
	mutex_destroy(&vfsspi_device->kernel_lock);
vfsspi_probe_parse_dt_failed:
	kfree(vfsspi_device);
	pr_err("%s failed!!\n", __func__);
	return status;
}

static int vfsspi_remove(struct spi_device *spi)
{
	int status = 0;

	struct vfsspi_device_data *vfsspi_device = NULL;

	pr_info("%s\n", __func__);

	vfsspi_device = spi_get_drvdata(spi);

	if (vfsspi_device != NULL) {
		vfsspi_disable_debug_timer();
		spin_lock_irq(&vfsspi_device->vfs_spi_lock);
		vfsspi_device->spi = NULL;
		spi_set_drvdata(spi, NULL);
		spin_unlock_irq(&vfsspi_device->vfs_spi_lock);

		mutex_lock(&device_list_mutex);

		vfsspi_uninit_platform(vfsspi_device);

		fingerprint_unregister(vfsspi_device->fp_device, fp_attrs);
		/* Remove device entry. */
		list_del(&vfsspi_device->device_entry);
		device_destroy(vfsspi_device_class, vfsspi_device->devt);
		class_destroy(vfsspi_device_class);
		cdev_del(&(vfsspi_device->cdev));
		unregister_chrdev_region(vfsspi_device->devt, 1);

		mutex_destroy(&vfsspi_device->buffer_mutex);
		mutex_destroy(&vfsspi_device->kernel_lock);

		kfree(vfsspi_device);
		mutex_unlock(&device_list_mutex);
	}

	return status;
}

static void vfsspi_shutdown(struct spi_device *spi)
{
	if (g_data != NULL)
		vfsspi_disable_debug_timer();
	pr_info("%s\n", __func__);
}

static int vfsspi_pm_suspend(struct device *dev)
{
	pr_info("%s\n", __func__);
	if (g_data != NULL) {
		vfsspi_disable_debug_timer();
#ifdef ENABLE_SENSORS_FPRINT_SECURE
		fpsensor_goto_suspend = 1; /* used by pinctrl_samsung.c */
		if (g_data->detect_mode &&
				(atomic_read(&g_data->irq_enabled) == DRDY_IRQ_ENABLE)) {
			pr_info("%s: suspend smc ret(wof)=%d\n", __func__,
				exynos_smc(MC_FC_FP_PM_SUSPEND_RETAIN, 0, 0, 0));
		} else {
			pr_info("%s: suspend smc ret=%d\n", __func__,
				exynos_smc(MC_FC_FP_PM_SUSPEND, 0, 0, 0));
		}
#endif
	}
	return 0;
}

static int vfsspi_pm_resume(struct device *dev)
{
	pr_info("%s\n", __func__);
	if (g_data != NULL)
		vfsspi_enable_debug_timer();
	return 0;
}

static const struct dev_pm_ops vfsspi_pm_ops = {
	.suspend = vfsspi_pm_suspend,
	.resume = vfsspi_pm_resume
};

struct spi_driver vfsspi_spi = {
	.driver = {
		   .name = VALIDITY_PART_NAME,
		   .owner = THIS_MODULE,
		   .pm = &vfsspi_pm_ops,
		   .of_match_table = vfsspi_match_table,
		   },
	.probe = vfsspi_probe,
	.remove = vfsspi_remove,
	.shutdown = vfsspi_shutdown,
};

static int __init vfsspi_init(void)
{
	int status = 0;

	pr_info("%s\n", __func__);

	status = spi_register_driver(&vfsspi_spi);
	if (status < 0) {
		pr_err("%s spi_register_driver() failed\n", __func__);
		return status;
	}
	pr_info("%s init is successful\n", __func__);

	return status;
}

static void __exit vfsspi_exit(void)
{
	pr_debug("%s\n", __func__);
	spi_unregister_driver(&vfsspi_spi);
}

module_init(vfsspi_init);
module_exit(vfsspi_exit);

MODULE_DESCRIPTION("Synaptics FP sensor");
MODULE_LICENSE("GPL");

