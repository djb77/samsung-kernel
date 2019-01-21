/*
 * Copyright (C) 2010 Trusted Logic S.A.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include "pn547.h"
#include <linux/wakelock.h>
#include <linux/of_gpio.h>
#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
#include <mach/msm_xo.h>
#include <linux/workqueue.h>
#endif
#include <linux/clk.h>
#include <linux/regulator/consumer.h>

#include <asm/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/signal.h>

#define SIG_NFC 44
#define MAX_BUFFER_SIZE		512
#define NFC_DEBUG 0
#define MAX_TRY_I2C_READ	10
#define I2C_ADDR_READ_L		0x51
#define I2C_ADDR_READ_H		0x57

#define FEATURE_SEC_NFC_TEST

struct pn547_dev {
	wait_queue_head_t read_wq;
	struct mutex read_mutex;
	struct i2c_client *client;
	struct miscdevice pn547_device;
	void (*conf_gpio) (void);
	unsigned int ven_gpio;
	unsigned int firm_gpio;
	unsigned int irq_gpio;
	int pvdd; /*pvdd-gpio number*/
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	unsigned int ese_pwr_req;
    struct mutex        p61_state_mutex; /* used to make p61_current_state flag secure */
    p61_access_state_t  p61_current_state; /* stores the current P61 state */
#endif
    bool                spi_ven_enabled; /* stores the VEN pin state powered by Spi */
    bool                nfc_ven_enabled; /* stores the VEN pin state powered by Nfc */

	atomic_t irq_enabled;
	atomic_t read_flag;
	bool cancel_read;
	struct wake_lock nfc_wake_lock;
	struct clk *nfc_clock;
	unsigned int clk_req_gpio;
		
#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
	unsigned int clk_req_gpio;
	unsigned int clk_req_irq;
	struct msm_xo_voter *nfc_clock;
	struct work_struct work_nfc_clock;
	struct workqueue_struct *wq_clock;
	bool clock_state;
#endif
	long	nfc_service_pid; /*used to signal the nfc the nfc service */
};

static struct pn547_dev *pn547_dev;

#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
static struct semaphore ese_access_sema;
static void release_ese_lock(p61_access_state_t  p61_current_state);
static struct semaphore svdd_sync_onoff_sema;
int get_ese_lock(p61_access_state_t  p61_current_state, int timeout);
static unsigned char svdd_sync_wait;
#endif

static irqreturn_t pn547_dev_irq_handler(int irq, void *dev_id)
{
	struct pn547_dev *pn547_dev = dev_id;

	if (!gpio_get_value(pn547_dev->irq_gpio)) {
#if NFC_DEBUG
		pr_err("%s, irq_gpio = %d\n", __func__,
			gpio_get_value(pn547_dev->irq_gpio));
#endif
		return IRQ_HANDLED;
	}

	/* Wake up waiting readers */
	atomic_set(&pn547_dev->read_flag, 1);
	wake_up(&pn547_dev->read_wq);


#if NFC_DEBUG
	pr_info("pn547 : call\n");
#endif
	wake_lock_timeout(&pn547_dev->nfc_wake_lock, 2*HZ);
	return IRQ_HANDLED;
}

#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
static void nfc_work_func_clock(struct work_struct *work)
{
	struct pn547_dev *pn547_dev = container_of(work, struct pn547_dev,
					      work_nfc_clock);
	int ret = 0;

	if (gpio_get_value(pn547_dev->clk_req_gpio)) {
		if (pn547_dev->clock_state == false) {
			ret = msm_xo_mode_vote(pn547_dev->nfc_clock,
						MSM_XO_MODE_ON);
			if (ret < 0) {
				pr_err("%s:  Failed to vote for TCX0_A1 ON (%d)\n",
						__func__, ret);
			}
			pn547_dev->clock_state = true;
		}
	} else {
		if (pn547_dev->clock_state == true) {
			ret = msm_xo_mode_vote(pn547_dev->nfc_clock,
						MSM_XO_MODE_OFF);
			if (ret < 0) {
				pr_err("%s:  Failed to vote for TCX0_A1 OFF (%d)\n",
						__func__, ret);
			}
			pn547_dev->clock_state = false;
		}
	}
}

static irqreturn_t pn547_dev_clk_req_irq_handler(int irq, void *dev_id)
{
	struct pn547_dev *pn547_dev = dev_id;
	queue_work(pn547_dev->wq_clock, &pn547_dev->work_nfc_clock);
	return IRQ_HANDLED;
}
#endif

static ssize_t pn547_dev_read(struct file *filp, char __user *buf,
			      size_t count, loff_t *offset)
{
	struct pn547_dev *pn547_dev = filp->private_data;
	char tmp[MAX_BUFFER_SIZE] = {0, };
	int ret = 0;
#ifdef CONFIG_NFC_PN544
	int readingWatchdog = 0;
#endif

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	pr_debug("%s : reading %zu bytes. irq=%s\n", __func__, count,
		 gpio_get_value(pn547_dev->irq_gpio) ? "1" : "0");

#if NFC_DEBUG
	pr_info("pn547 : + r\n");
#endif

	mutex_lock(&pn547_dev->read_mutex);

#ifdef CONFIG_NFC_PN544
wait_irq:
#endif

	if (!gpio_get_value(pn547_dev->irq_gpio)) {
		atomic_set(&pn547_dev->read_flag, 0);
		if (filp->f_flags & O_NONBLOCK) {
			pr_info("%s : O_NONBLOCK\n", __func__);
			ret = -EAGAIN;
			goto fail;
		}

#if NFC_DEBUG
		pr_info("pn547: wait_event_interruptible : in\n");
#endif
		if (!gpio_get_value(pn547_dev->irq_gpio))
			ret = wait_event_interruptible(pn547_dev->read_wq,
				atomic_read(&pn547_dev->read_flag));

#if NFC_DEBUG
		pr_info("pn547 :   h\n");
#endif

		if (pn547_dev->cancel_read) {
			pn547_dev->cancel_read = false;
			ret = -1;
			goto fail;
		}

		if (ret)
			goto fail;

	}

	/* Read data */
	ret = i2c_master_recv(pn547_dev->client, tmp, count);

#ifdef CONFIG_NFC_PN544
	/* If bad frame(from 0x51 to 0x57) is received from pn65n,
	* we need to read again after waiting that IRQ is down.
	* if data is not ready, pn65n will send from 0x51 to 0x57. */
	if ((I2C_ADDR_READ_L <= tmp[0] && tmp[0] <= I2C_ADDR_READ_H)
		&& readingWatchdog < MAX_TRY_I2C_READ) {
		pr_warn("%s: data is not ready yet.data = 0x%x, cnt=%d\n",
			__func__, tmp[0], readingWatchdog);
		usleep_range(2000, 2000); /* sleep 2ms to wait for IRQ */
		readingWatchdog++;
		goto wait_irq;
	}
#endif

#if NFC_DEBUG
	pr_info("pn547: i2c_master_recv\n");
#endif
	mutex_unlock(&pn547_dev->read_mutex);
	if (ret < 0) {
		pr_err("%s: i2c_master_recv returned %d\n", __func__,
				ret);
		return ret;
	}

	if (ret > count) {
		pr_err("%s: received too many bytes from i2c (%d)\n",
				__func__, ret);
		return -EIO;
	}

	if (copy_to_user(buf, tmp, ret)) {
		pr_err("%s : failed to copy to user space\n", __func__);
		return -EFAULT;
	}
	return ret;

fail:
	mutex_unlock(&pn547_dev->read_mutex);
	return ret;
}

static ssize_t pn547_dev_write(struct file *filp, const char __user *buf,
			       size_t count, loff_t *offset)
{
	struct pn547_dev *pn547_dev;
	char tmp[MAX_BUFFER_SIZE] = {0, };
	int ret = 0, retry = 2;

	pn547_dev = filp->private_data;

#if NFC_DEBUG
	pr_info("pn547 : + w\n");
#endif

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (copy_from_user(tmp, buf, count)) {
		pr_err("%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}

	pr_debug("%s : writing %zu bytes.\n", __func__, count);
	/* Write data */
	do {
		retry--;
		ret = i2c_master_send(pn547_dev->client, tmp, count);
		if (ret == count)
			break;
		usleep_range(6000, 10000); /* Retry, chip was in standby */
#if NFC_DEBUG
		pr_debug("%s : retry = %d\n", __func__, retry);
#endif
	} while (retry);

#if NFC_DEBUG
	pr_info("pn547 : - w\n");
#endif

	if (ret != count) {
		pr_err("%s : i2c_master_send returned %d\n", __func__, ret);
		ret = -EIO;
	}

	return ret;
}

#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
static void p61_update_access_state(struct pn547_dev *pn547_dev, p61_access_state_t current_state, bool set)
{
    pr_info("%s: Enter current_state = 0x%x\n", __func__, pn547_dev->p61_current_state);
    if (current_state)
    {
        if(set){
            if(pn547_dev->p61_current_state == P61_STATE_IDLE)
                pn547_dev->p61_current_state = P61_STATE_INVALID;
            pn547_dev->p61_current_state |= current_state;
        }
        else{
            pn547_dev->p61_current_state ^= current_state;
            if(!pn547_dev->p61_current_state)
                pn547_dev->p61_current_state = P61_STATE_IDLE;
        }
    }
    pr_info("%s: Exit current_state = 0x%x\n", __func__, pn547_dev->p61_current_state);
}

static void p61_get_access_state(struct pn547_dev *pn547_dev, p61_access_state_t *current_state)
{

    if (current_state == NULL) {
        pr_err("%s : invalid state of p61_access_state_t current state  \n", __func__);
    } else {
        *current_state = pn547_dev->p61_current_state;
    }
}
static void p61_access_lock(struct pn547_dev *pn547_dev)
{
    pr_info("%s: Enter\n", __func__);
    mutex_lock(&pn547_dev->p61_state_mutex);
    pr_info("%s: Exit\n", __func__);
}
static void p61_access_unlock(struct pn547_dev *pn547_dev)
{
    pr_info("%s: Enter\n", __func__);
    mutex_unlock(&pn547_dev->p61_state_mutex);
    pr_info("%s: Exit\n", __func__);
}

static int signal_handler(p61_access_state_t state, long nfc_pid)
{
    struct siginfo sinfo;
    pid_t pid;
    struct task_struct *task;
    int sigret = 0;
    int ret = 0;

    pr_info("%s: Enter\n", __func__);

    memset(&sinfo, 0, sizeof(struct siginfo));
    sinfo.si_signo = SIG_NFC;
    sinfo.si_code = SI_QUEUE;
    sinfo.si_int = state;
    pid = nfc_pid;

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if(task)
    {
        pr_info("%s.\n", task->comm);
        sigret = send_sig_info(SIG_NFC, &sinfo, task);
        if(sigret < 0){
            pr_info("send_sig_info failed..... sigret %d.\n", sigret);
		return -1;
        }
    }
    else{
        pr_info("finding task from PID failed\r\n");
	return -1;
    }
    pr_info("%s: Exit\n", __func__);

	return ret;
}

static void svdd_sync_onoff(long nfc_service_pid, p61_access_state_t origin)
{
    int timeout = 100; //100 ms timeout
    unsigned long tempJ = msecs_to_jiffies(timeout);
    pr_info("%s: Enter nfc_service_pid: %ld\n", __func__, nfc_service_pid);
    if(nfc_service_pid)
    {
        if (0 == signal_handler(origin, nfc_service_pid))
        {
            sema_init(&svdd_sync_onoff_sema, 0);
	    svdd_sync_wait = 1;
            pr_info("Waiting for svdd protection response");
            if(down_timeout(&svdd_sync_onoff_sema, tempJ) != 0)
            {
                pr_info("svdd wait protection: Timeout");
            }
            pr_info("svdd wait protection : released");
	    svdd_sync_wait = 0;
        }
    }
    pr_info("%s: Exit\n", __func__);
}

static int release_svdd_wait(void)
{
    unsigned char i = 0;
    pr_info("%s: Enter \n", __func__);
    
    for(i=0;i<9;i++)
    {
        if(svdd_sync_wait)
        {
            up(&svdd_sync_onoff_sema);
            svdd_sync_wait = 0;
            break;
        } else msleep(10);
    }
    pr_info("%s: Exit\n", __func__);
    return 0;
}

#endif

static int pn547_dev_open(struct inode *inode, struct file *filp)
{
	struct pn547_dev *pn547_dev = container_of(filp->private_data,
						   struct pn547_dev,
						   pn547_device);
	filp->private_data = pn547_dev;

	pr_debug("%s : %d,%d\n", __func__, imajor(inode), iminor(inode));

	return 0;
}

#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
static unsigned char p61_trans_acc_on = 0;
#endif
long pn547_dev_ioctl(struct file *filp,
			   unsigned int cmd, unsigned long arg)
{
	/*struct pn547_dev *pn547_dev = filp->private_data;*/
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	if (cmd == P547_GET_ESE_ACCESS)
	{
		return get_ese_lock(P61_STATE_WIRED, arg);
	}
	else if(cmd == P547_REL_SVDD_WAIT)
	{
		return release_svdd_wait();
	}

	p61_access_lock(pn547_dev);
	switch (cmd) {
	case PN547_SET_PWR:
	{
		p61_access_state_t current_state = P61_STATE_INVALID;
		p61_get_access_state(pn547_dev, &current_state);

		if (arg == 2) {
			if (current_state & (P61_STATE_SPI|P61_STATE_SPI_PRIO))
			{
			    /* NFCC fw/download should not be allowed if p61 is used
			     * by SPI
			     */
			    pr_info("%s NFCC should not be allowed to reset/FW download \n", __func__);
			    p61_access_unlock(pn547_dev);
			    return -EBUSY; /* Device or resource busy */
			}
			pn547_dev->nfc_ven_enabled = true;
			if (pn547_dev->spi_ven_enabled == false)
			{
				/* power on with firmware download (requires hw reset)
				 */
				p61_update_access_state(pn547_dev, P61_STATE_DWNLD, true);
				gpio_set_value_cansleep(pn547_dev->ven_gpio, 1);
				gpio_set_value(pn547_dev->firm_gpio, 1);
				usleep_range(4900, 5000);
				gpio_set_value_cansleep(pn547_dev->ven_gpio, 0);
				usleep_range(4900, 5000);
				gpio_set_value_cansleep(pn547_dev->ven_gpio, 1);
				usleep_range(4900, 5000);
				if (atomic_read(&pn547_dev->irq_enabled) == 0) {
					atomic_set(&pn547_dev->irq_enabled, 1);
					enable_irq(pn547_dev->client->irq);
					enable_irq_wake(pn547_dev->client->irq);
				}
				pr_info("%s power on with firmware, irq=%d\n", __func__,
				atomic_read(&pn547_dev->irq_enabled));

				pr_info("VEN=%d FIRM=%d\n", gpio_get_value(pn547_dev->ven_gpio), gpio_get_value(pn547_dev->firm_gpio));
			}
		} else if (arg == 1) {
			/* power on */
			if (pn547_dev->conf_gpio)
				pn547_dev->conf_gpio();
			if ((current_state & (P61_STATE_WIRED|P61_STATE_SPI|P61_STATE_SPI_PRIO))== 0){
                    p61_update_access_state(pn547_dev, P61_STATE_IDLE, true);
			}
			gpio_set_value(pn547_dev->firm_gpio, 0);

			pn547_dev->nfc_ven_enabled = true;
			if (pn547_dev->spi_ven_enabled == false) {
				gpio_set_value_cansleep(pn547_dev->ven_gpio, 1);
			}
			usleep_range(4900, 5000);
			if (atomic_read(&pn547_dev->irq_enabled) == 0) {
				atomic_set(&pn547_dev->irq_enabled, 1);
				enable_irq(pn547_dev->client->irq);
				enable_irq_wake(pn547_dev->client->irq);
			}

			svdd_sync_wait = 0;

			pr_info("%s power on, irq=%d\n", __func__,
				atomic_read(&pn547_dev->irq_enabled));
		} else if (arg == 0) {
			/* power off */
			if (atomic_read(&pn547_dev->irq_enabled) == 1) {
				atomic_set(&pn547_dev->irq_enabled, 0);
				disable_irq_wake(pn547_dev->client->irq);
				disable_irq_nosync(pn547_dev->client->irq);
			}
			if(current_state & P61_STATE_DWNLD) 
				p61_update_access_state(pn547_dev, P61_STATE_DWNLD, false);

			if ((current_state & (P61_STATE_WIRED|P61_STATE_SPI|P61_STATE_SPI_PRIO))== 0){
				p61_update_access_state(pn547_dev, P61_STATE_IDLE, true);
			}
			pr_info("%s power off, irq=%d\n", __func__,
				atomic_read(&pn547_dev->irq_enabled));
			gpio_set_value(pn547_dev->firm_gpio, 0);

			pn547_dev->nfc_ven_enabled = false;
			/* Don't change Ven state if spi made it high */
			if (pn547_dev->spi_ven_enabled == false) {
				gpio_set_value_cansleep(pn547_dev->ven_gpio, 0);
			}
			usleep_range(4900, 5000);
		} else if (arg == 3) {
			pr_info("%s Read Cancel\n", __func__);
			pn547_dev->cancel_read = true;
			atomic_set(&pn547_dev->read_flag, 1);
			wake_up(&pn547_dev->read_wq);
		} else {
			pr_err("%s bad arg %lu\n", __func__, arg);
			/* changed the p61 state to idle*/
			p61_access_unlock(pn547_dev);
			return -EINVAL;
		}
	}
	break;

	case P61_SET_SPI_PWR:
	{
		p61_access_state_t current_state = P61_STATE_INVALID;
		p61_get_access_state(pn547_dev, &current_state);
		if (arg == 1) {
			pr_info("%s : PN61_SET_SPI_PWR - power on ese, cur=0x%x\n", __func__, current_state);

			if ((current_state & (P61_STATE_SPI|P61_STATE_SPI_PRIO|P61_STATE_DWNLD)) == 0)
			{
				p61_update_access_state(pn547_dev, P61_STATE_SPI, true);
				/*To handle triple mode protection signal
				  NFC service when SPI session started*/
				if (current_state & P61_STATE_WIRED){
					if(pn547_dev->nfc_service_pid){
						pr_info("nfc service pid %s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
						signal_handler(P61_STATE_SPI, pn547_dev->nfc_service_pid);
					}
					else{
						pr_info(" invalid nfc service pid....signalling failed%s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
					}
				}
				pn547_dev->spi_ven_enabled = true;
				if (pn547_dev->nfc_ven_enabled == false) {
					/* provide power to NFCC if, NFC service not provided */
					gpio_set_value(pn547_dev->ven_gpio, 1);
					msleep(10);
				}
				/* pull the gpio to high once NFCC is power on*/
				gpio_set_value(pn547_dev->ese_pwr_req, 1);
				msleep(10);
			} else {
				pr_info("%s : PN61_SET_SPI_PWR -  power on ese failed \n", __func__);
				p61_access_unlock(pn547_dev);
				return -EBUSY; /* Device or resource busy */
			}
		} else if (arg == 0) {
			pr_info("%s : PN61_SET_SPI_PWR - power off ese\n", __func__);
			if(current_state & P61_STATE_SPI_PRIO){
				p61_update_access_state(pn547_dev, P61_STATE_SPI_PRIO, false);
				if (current_state & P61_STATE_WIRED)
				{
					if(pn547_dev->nfc_service_pid){
						pr_info("nfc service pid %s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
						signal_handler(P61_STATE_SPI_PRIO_END, pn547_dev->nfc_service_pid);
					}else{
						pr_info(" invalid nfc service pid....signalling failed%s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
					}
				}
				if (!(current_state & P61_STATE_WIRED)) {
					svdd_sync_onoff(pn547_dev->nfc_service_pid, P61_STATE_SPI_SVDD_SYNC_START);
					gpio_set_value(pn547_dev->ese_pwr_req, 0);
					msleep(60);
					svdd_sync_onoff(pn547_dev->nfc_service_pid, P61_STATE_SPI_SVDD_SYNC_END);
				}

				pn547_dev->spi_ven_enabled = false;

				if (pn547_dev->nfc_ven_enabled == false) {
					gpio_set_value(pn547_dev->ven_gpio, 0);
					msleep(10);
				}
			}

			else if(current_state & P61_STATE_SPI){
				p61_update_access_state(pn547_dev, P61_STATE_SPI, false);
				if (!(current_state & P61_STATE_WIRED))
				{
					svdd_sync_onoff(pn547_dev->nfc_service_pid, P61_STATE_SPI_SVDD_SYNC_START);
					gpio_set_value(pn547_dev->ese_pwr_req, 0);
					msleep(60);
					svdd_sync_onoff(pn547_dev->nfc_service_pid, P61_STATE_SPI_SVDD_SYNC_END);
				}
				/*If JCOP3.2 or 3.3 for handling triple mode protection signal NFC service */
				else
				{
					if (current_state & P61_STATE_WIRED)
					{
						if(pn547_dev->nfc_service_pid){
							pr_info("nfc service pid %s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
							signal_handler(P61_STATE_SPI_END, pn547_dev->nfc_service_pid);
						}
						else{
							pr_info(" invalid nfc service pid....signalling failed%s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
						}
					}
				}
				pn547_dev->spi_ven_enabled = false;
				if (pn547_dev->nfc_ven_enabled == false) {
					gpio_set_value(pn547_dev->ven_gpio, 0);
					msleep(10);
				}
			} else {
				pr_err("%s : PN61_SET_SPI_PWR - failed, current_state = 0x%x \n",
						__func__, pn547_dev->p61_current_state);
				p61_access_unlock(pn547_dev);
				return -EPERM; /* Operation not permitted */
			}
		}else if (arg == 2) {
			pr_info("%s : PN61_SET_SPI_PWR - reset\n", __func__);
			if (current_state & (P61_STATE_IDLE|P61_STATE_SPI|P61_STATE_SPI_PRIO)) {
				if (pn547_dev->spi_ven_enabled == false)
				{
					pn547_dev->spi_ven_enabled = true;
					if (pn547_dev->nfc_ven_enabled == false) {
						/* provide power to NFCC if, NFC service not provided */
						gpio_set_value(pn547_dev->ven_gpio, 1);
						msleep(10);
					}
				}
				svdd_sync_onoff(pn547_dev->nfc_service_pid, P61_STATE_SPI_SVDD_SYNC_START);
				gpio_set_value(pn547_dev->ese_pwr_req, 0);
				msleep(60);
				svdd_sync_onoff(pn547_dev->nfc_service_pid, P61_STATE_SPI_SVDD_SYNC_END);
				gpio_set_value(pn547_dev->ese_pwr_req, 1);
				msleep(10);
			} else {
				pr_info("%s : PN61_SET_SPI_PWR - reset  failed \n", __func__);
				p61_access_unlock(pn547_dev);
				return -EBUSY; /* Device or resource busy */
			}
		}

		else if (arg == 3) {
			pr_info("%s : PN61_SET_SPI_PWR - Prio Session Start power on ese\n", __func__);
			if ((current_state & (P61_STATE_SPI|P61_STATE_SPI_PRIO|P61_STATE_DWNLD)) == 0) {
				p61_update_access_state(pn547_dev, P61_STATE_SPI_PRIO, true);
				if (current_state & P61_STATE_WIRED){
					if(pn547_dev->nfc_service_pid){
						pr_info("nfc service pid %s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
						signal_handler(P61_STATE_SPI_PRIO, pn547_dev->nfc_service_pid);
					}
					else{
						pr_info(" invalid nfc service pid....signalling failed%s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
					}
				}
				pn547_dev->spi_ven_enabled = true;
				if (pn547_dev->nfc_ven_enabled == false) {
					/* provide power to NFCC if, NFC service not provided */
					gpio_set_value(pn547_dev->ven_gpio, 1);
					msleep(10);
				}
				/* pull the gpio to high once NFCC is power on*/
				gpio_set_value(pn547_dev->ese_pwr_req, 1);
				msleep(10);
			}else {
				pr_info("%s : Prio Session Start power on ese failed 0x%x\n", __func__, current_state);
				p61_access_unlock(pn547_dev);
				return -EBUSY; /* Device or resource busy */
			}
		}else if (arg == 4) {
			if (current_state & P61_STATE_SPI_PRIO)
			{
				pr_info("%s : PN61_SET_SPI_PWR - Prio Session Ending...\n", __func__);
				p61_update_access_state(pn547_dev, P61_STATE_SPI_PRIO, false);
				/*after SPI prio timeout, the state is changing from SPI prio to SPI */
				p61_update_access_state(pn547_dev, P61_STATE_SPI, true);
				if (current_state & P61_STATE_WIRED)
				{
					if(pn547_dev->nfc_service_pid){
						pr_info("nfc service pid %s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
						signal_handler(P61_STATE_SPI_PRIO_END, pn547_dev->nfc_service_pid);
					}
					else{
						pr_info(" invalid nfc service pid....signalling failed%s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
					}
				}
			}
			else
			{
				pr_info("%s : PN61_SET_SPI_PWR -  Prio Session End failed 0x%x\n", __func__, current_state);
				p61_access_unlock(pn547_dev);
				return -EBADRQC; /* Device or resource busy */
			}
		} else if(arg == 5){
			release_ese_lock(P61_STATE_SPI);
		}
		else {
			pr_info("%s bad ese pwr arg %lu\n", __func__, arg);
			p61_access_unlock(pn547_dev);
			return -EBADRQC; /* Invalid request code */
		}
	}
	break;

	case P61_GET_PWR_STATUS:
	{
		p61_access_state_t current_state = P61_STATE_INVALID;
		p61_get_access_state(pn547_dev, &current_state);
		pr_info("%s: P61_GET_PWR_STATUS  = %x",__func__, current_state);
        put_user(current_state, (int __user *)arg);
	}
	break;

	case P61_SET_WIRED_ACCESS:
	{
		p61_access_state_t current_state = P61_STATE_INVALID;
		p61_get_access_state(pn547_dev, &current_state);
		if (arg == 1) {
        	if (current_state)
            {
			pr_info("%s : P61_SET_WIRED_ACCESS - enabling\n", __func__);
                p61_update_access_state(pn547_dev, P61_STATE_WIRED, true);
                if (current_state & P61_STATE_SPI_PRIO)
                {
                    if(pn547_dev->nfc_service_pid){
                        pr_info("nfc service pid %s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
                        signal_handler(P61_STATE_SPI_PRIO, pn547_dev->nfc_service_pid);
                    }
                    else{
                        pr_info(" invalid nfc service pid....signalling failed%s   ---- %ld", __func__, pn547_dev->nfc_service_pid);
                    }
                }
                if((current_state & (P61_STATE_SPI|P61_STATE_SPI_PRIO)) == 0) {
                    gpio_set_value(pn547_dev->ese_pwr_req, 1);
			msleep(10);
		}

			} else {
	                pr_info("%s : P61_SET_WIRED_ACCESS -  enabling failed \n", __func__);
				p61_access_unlock(pn547_dev);
				return -EBUSY; /* Device or resource busy */
			}
		} else if (arg == 0) {
			pr_info("%s : P61_SET_WIRED_ACCESS - disabling \n", __func__);
			if (current_state & P61_STATE_WIRED){
				p61_update_access_state(pn547_dev, P61_STATE_WIRED, false);
				if((current_state & (P61_STATE_SPI|P61_STATE_SPI_PRIO)) == 0) {
					svdd_sync_onoff(pn547_dev->nfc_service_pid, P61_STATE_DWP_SVDD_SYNC_START);
					gpio_set_value(pn547_dev->ese_pwr_req, 0);
					msleep(60);
					svdd_sync_onoff(pn547_dev->nfc_service_pid, P61_STATE_DWP_SVDD_SYNC_END);
				}
			} else {
                pr_err("%s : P61_SET_WIRED_ACCESS - failed, current_state = %x \n",
						__func__, pn547_dev->p61_current_state);
				p61_access_unlock(pn547_dev);
				return -EPERM; /* Operation not permitted */
			}
		}
		else if(arg == 2)
		{
			pr_info("%s : P61 ESE POWER REQ LOW  \n", __func__);
			svdd_sync_onoff(pn547_dev->nfc_service_pid, P61_STATE_DWP_SVDD_SYNC_START);
			gpio_set_value(pn547_dev->ese_pwr_req, 0);
			msleep(60);
			svdd_sync_onoff(pn547_dev->nfc_service_pid, P61_STATE_DWP_SVDD_SYNC_END);
		}
		else if(arg == 3)
		{
			pr_info("%s : P61 ESE POWER REQ HIGH  \n", __func__);
			gpio_set_value(pn547_dev->ese_pwr_req, 1);
			msleep(10);
		}
		else if(arg == 4)
		{
			release_ese_lock(P61_STATE_WIRED);
		}
		else {
			pr_info("%s P61_SET_WIRED_ACCESS - bad arg %lu\n", __func__, arg);
			p61_access_unlock(pn547_dev);
			return -EBADRQC; /* Invalid request code */
		}
	}
	break;

	case P547_SET_NFC_SERVICE_PID:
    {
		p61_access_state_t current_state = P61_STATE_INVALID;
		p61_get_access_state(pn547_dev, &current_state);
        if((p61_trans_acc_on ==  1) && ((current_state & (P61_STATE_SPI|P61_STATE_SPI_PRIO)) == 0))
            release_ese_lock(P61_STATE_WIRED);

        pr_info("%s : The NFC Service PID is %ld\n", __func__, arg);
        pn547_dev->nfc_service_pid = arg;
    }
    break;
	
	default:
		pr_err("%s bad ioctl %u\n", __func__, cmd);
		p61_access_unlock(pn547_dev);
		pr_info("%s :exit cmd = %u, arg = %ld\n", __func__, cmd, arg);
		return -EINVAL;
	}
	p61_access_unlock(pn547_dev);
#else
	switch (cmd) {
	case PN547_SET_PWR:
		if (arg == 2) {
			/* power on with firmware download (requires hw reset)
			 */
			gpio_set_value_cansleep(pn547_dev->ven_gpio, 1);
			gpio_set_value(pn547_dev->firm_gpio, 1);
			usleep_range(4900, 5000);
			gpio_set_value_cansleep(pn547_dev->ven_gpio, 0);
			usleep_range(4900, 5000);
			gpio_set_value_cansleep(pn547_dev->ven_gpio, 1);
			usleep_range(4900, 5000);
			if (atomic_read(&pn547_dev->irq_enabled) == 0) {
				atomic_set(&pn547_dev->irq_enabled, 1);
				enable_irq(pn547_dev->client->irq);
				enable_irq_wake(pn547_dev->client->irq);
			}
			pr_info("%s power on with firmware, irq=%d\n", __func__,
				atomic_read(&pn547_dev->irq_enabled));
		} else if (arg == 1) {
			/* power on */
			if (pn547_dev->conf_gpio)
				pn547_dev->conf_gpio();
			gpio_set_value(pn547_dev->firm_gpio, 0);
			gpio_set_value_cansleep(pn547_dev->ven_gpio, 1);
			usleep_range(4900, 5000);
			if (atomic_read(&pn547_dev->irq_enabled) == 0) {
				atomic_set(&pn547_dev->irq_enabled, 1);
				enable_irq(pn547_dev->client->irq);
				enable_irq_wake(pn547_dev->client->irq);
			}
			pr_info("%s power on, irq=%d\n", __func__,
				atomic_read(&pn547_dev->irq_enabled));
		} else if (arg == 0) {
			/* power off */
			if (atomic_read(&pn547_dev->irq_enabled) == 1) {
				atomic_set(&pn547_dev->irq_enabled, 0);
				disable_irq_wake(pn547_dev->client->irq);
				disable_irq_nosync(pn547_dev->client->irq);
			}
			pr_info("%s power off, irq=%d\n", __func__,
				atomic_read(&pn547_dev->irq_enabled));
			gpio_set_value(pn547_dev->firm_gpio, 0);
	#ifdef CONFIG_SEC_MILLETWIFI_COMMON
			gpio_direction_output(pn547_dev->ven_gpio, 0);
	#endif
			gpio_set_value_cansleep(pn547_dev->ven_gpio, 0);
			usleep_range(4900, 5000);
		} else if (arg == 3) {
			pr_info("%s Read Cancel\n", __func__);
			pn547_dev->cancel_read = true;
			atomic_set(&pn547_dev->read_flag, 1);
			wake_up(&pn547_dev->read_wq);
		} else {
			pr_err("%s bad arg %lu\n", __func__, arg);
			return -EINVAL;
		}
		break;
	default:
		pr_err("%s bad ioctl %u\n", __func__, cmd);
		return -EINVAL;
	}
#endif
	return 0;
}

EXPORT_SYMBOL(pn547_dev_ioctl);

#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
int get_ese_lock(p61_access_state_t  p61_current_state, int timeout)
{
	unsigned long tempJ = msecs_to_jiffies(timeout);
	pr_info("get_ese_lock: enter p61_current_state =(0x%x), timeout = %d, jiffies = %lu\n"
			, p61_current_state, timeout, tempJ);
	if(down_timeout(&ese_access_sema, tempJ) != 0)
	{
		pr_err("get_ese_lock: timeout p61_current_state = %d\n", p61_current_state);
		return -EBUSY;
	}
    p61_trans_acc_on = 1;
	pr_info("get_ese_lock: exit p61_trans_acc_on =%d, timeout = %d\n"
			, p61_trans_acc_on, timeout);
	return 0;
}
EXPORT_SYMBOL(get_ese_lock);

static void release_ese_lock(p61_access_state_t  p61_current_state)
{
	pr_info("%s: enter p61_current_state = (0x%x)\n", __func__, p61_current_state);
	up(&ese_access_sema);
    p61_trans_acc_on = 0;
	pr_info("%s: p61_trans_acc_on =%d exit\n", __func__, p61_trans_acc_on);
}
#endif

static const struct file_operations pn547_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = pn547_dev_read,
	.write = pn547_dev_write,
	.open = pn547_dev_open,
	.unlocked_ioctl = pn547_dev_ioctl,
};

static int pn547_parse_dt(struct device *dev,
	struct pn547_i2c_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int ret;

	pdata->irq_gpio = of_get_named_gpio_flags(np, "pn547,irq-gpio",
		0, &pdata->irq_gpio_flags);

	pdata->ven_gpio = of_get_named_gpio_flags(np, "pn547,ven-gpio",
		0, &pdata->ven_gpio_flags);
	if (pdata->ven_gpio < 0)
		of_property_read_u32(np, "pn547,ven-expander-gpio",
			&pdata->ven_gpio);

	pdata->firm_gpio = of_get_named_gpio_flags(np, "pn547,firm-gpio",
		0, &pdata->firm_gpio_flags);
	if (pdata->firm_gpio < 0)
		of_property_read_u32(np, "pn547,firm-expander-gpio",
			&pdata->firm_gpio);

#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	pdata->ese_pwr_req = of_get_named_gpio_flags(np, "pn547,pwr_req",
		0, &pdata->ese_pwr_req_flags);
#endif

	if (of_get_property(dev->of_node, "pn547,nfc_pm_clk", NULL)) {
		pdata->clk = clk_get(dev, "rf_clk");
		if (IS_ERR(pdata->clk)) {
			pr_err("%s: Couldn't get rf_clk\n", __func__);
		} else {
			pr_info("%s : enable rf_clk\n", __func__);
			clk_prepare_enable(pdata->clk);
		}
	    ret = of_get_named_gpio(np, "pn547,clk_req-gpio", 0);
		if (ret < 0) {
			pr_info("%s : clk_req-gpio is not set", __func__);
		} else
			pdata->clk_req_gpio = ret;
	}

	pr_info("%s: irq : %d, ven : %d, firm : %d\n",
			__func__, pdata->irq_gpio, pdata->ven_gpio,
			pdata->firm_gpio);

	return 0;
}

static int pn547_regulator_onoff(struct i2c_client *client, int onoff)
{
	int rc = 0;
	struct regulator *regulator_i2c_1p8;

	pr_err("%s - onoff = %d\n", __func__, onoff);
	regulator_i2c_1p8 = regulator_get(&client->dev, "pn547,i2c_1p8");
	if (IS_ERR(regulator_i2c_1p8) || regulator_i2c_1p8 == NULL) {
		pr_err("%s - i2c_1p8 regulator_get fail\n", __func__);
		return -ENODEV;
	}

	pr_info("%s - onoff = %d\n", __func__, onoff);

	if (onoff == NFC_I2C_LDO_ON) {
		rc = regulator_enable(regulator_i2c_1p8);
		if (rc) {
			pr_err("%s - enable i2c_1p8 failed, rc=%d\n",
				__func__, rc);
			goto done;
		}
	} else {
		rc = regulator_disable(regulator_i2c_1p8);
		if (rc) {
			pr_err("%s - disable i2c_1p8 failed, rc=%d\n",
				__func__, rc);
			goto done;
		}
	}

	done:
	regulator_put(regulator_i2c_1p8);

	return rc;
}
#ifdef FEATURE_SEC_NFC_TEST
static ssize_t pn547_class_show(struct class * class, struct class_attribute *attr, char *buf) {

	int size;

	int ret = 0;
	int count = 4;
	char tmp[128] = {0x20, 0x00, 0x01, 0x00, };
	int retry;
	bool old_ven, old_irq;
	int old_read_value;

	pr_err("%s : start\n", __func__);

	/*TODO : nfc_ven_enabled should be capsulated with ESE_SUPPORT */
	old_ven = pn547_dev->nfc_ven_enabled;
	pr_info("%s : old_ven is %d\n", __func__, old_ven);
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	p61_access_lock(pn547_dev);
#endif
	retry = 20;
	if(!old_ven) {	/* if nfc status is off */
		pn547_dev->nfc_ven_enabled = true;

		if (pn547_dev->spi_ven_enabled == false) {
#ifdef CONFIG_SEC_MILLETWIFI_COMMON
			gpio_direction_output(pn547_dev->ven_gpio, 1);
#endif
			gpio_set_value_cansleep(pn547_dev->ven_gpio, 1);
		}
		usleep_range(4900, 5000);
	}
	else {	/* if nfc status is on */
		//wake up device
		gpio_set_value_cansleep(pn547_dev->ven_gpio, 0);
		usleep_range(4900, 5000);
		gpio_set_value_cansleep(pn547_dev->ven_gpio, 1);
		usleep_range(4900, 5000);
		//intercept i2c_master_recv
		pn547_dev->cancel_read = 1;
		atomic_set(&pn547_dev->read_flag, 1);
	}
	wake_up_all(&pn547_dev->read_wq);
	while(!mutex_trylock(&pn547_dev->read_mutex) && --retry) {
		usleep_range(15, 20);
	}
	if(!retry) {
		pr_err("%s : mutex_trylock failed. check pn547_dev_read()\n", __func__);
		ret = sprintf(buf, "test failed : device in use\n");
		goto fail_lock;
	}

	pr_info("%s : read_mutex locked. retry : %d\n", __func__, retry);

	atomic_set(&pn547_dev->read_flag, 0);
	pn547_dev->cancel_read = 0;
	old_irq = atomic_read(&pn547_dev->irq_enabled);
	pr_info("%s : old_irq is %d\n", __func__, old_irq);
	if(!old_irq) {
		atomic_set(&pn547_dev->irq_enabled, 1);
		enable_irq(pn547_dev->client->irq);
		enable_irq_wake(pn547_dev->client->irq);
		pr_info("%s power on, irq=%d\n", __func__, atomic_read(&pn547_dev->irq_enabled));
	}
	retry = 2;
	do {
		ret = i2c_master_send(pn547_dev->client, tmp, count);
		if(count == ret) {
			break;
		}
		pr_info("%s : i2c_master_send error. ret: %d, retry: %d\n", __func__, ret, retry);
		usleep_range(6000, 10000); /* Retry, chip was in standby */
	} while(retry--);

	if(ret != count) {
		pr_err("%s : failed. count error. send=%d, recv=%d\n", __func__, count, ret);
		ret = 0;
		goto fail;
	}

	pr_err("%s : send success. returned: %d\n", __func__, ret);

	//wait for reply


#if NFC_DEBUG
	pr_info("pn547: wait_event_interruptible : in\n");
#endif
	ret = 0;
	old_read_value = atomic_read(&pn547_dev->read_flag);
	pr_info("%s: read_flag %d, cancel_read %d", __func__, old_read_value, pn547_dev->cancel_read);
	if(!old_read_value) {
		ret = wait_event_interruptible(pn547_dev->read_wq,
				atomic_read(&pn547_dev->read_flag));
	}

#if NFC_DEBUG
	pr_info("pn547 :   h\n");
#endif

	if (pn547_dev->cancel_read) {
		pn547_dev->cancel_read = false;
		ret = 0;
		//todo : old_ven and old_irq rollback needed
		goto fail;
	}

	if (ret) {
		ret = 0;
		goto fail;
	}

/* Read data */
	count = 6;
	ret = i2c_master_recv(pn547_dev->client, tmp, count);

#if NFC_DEBUG
	pr_info("pn547: i2c_master_recv\n");
#endif
	mutex_unlock(&pn547_dev->read_mutex);

	if(!old_ven) {	/* if nfc status is off */
		if (pn547_dev->spi_ven_enabled == false) {
#ifdef CONFIG_SEC_MILLETWIFI_COMMON
			gpio_direction_output(pn547_dev->ven_gpio, 0);
#endif
			gpio_set_value_cansleep(pn547_dev->ven_gpio, 0);
		}
		usleep_range(4900, 5000);
		pn547_dev->nfc_ven_enabled = false;
	}
	if(!old_irq) {
		atomic_set(&pn547_dev->irq_enabled, 0);
		disable_irq_wake(pn547_dev->client->irq);
		disable_irq_nosync(pn547_dev->client->irq);
	}
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	p61_access_unlock(pn547_dev);
#endif
	if (ret < 0 || ret > count) {
		pr_err("%s: i2c_master_recv returned %d. count : %d\n", __func__, ret, count);
		return 0;
	}
	size = sprintf(buf, "test completed!! size: %d, data: %X %X %X %X %X %X\n", ret, tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);
	pr_err("%s : recv success.\n", __func__);

	msleep(10);

	return size;

fail:
	mutex_unlock(&pn547_dev->read_mutex);
fail_lock:
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	p61_access_unlock(pn547_dev);
#endif
	return ret;
}
static ssize_t pn547_class_store(struct class * class, struct class_attribute *attr, const char *buf, size_t size) {
	return size;
}
static CLASS_ATTR(test, 0664, pn547_class_show, pn547_class_store);

#endif //FEATURE_SEC_NFC_TEST

static int pn547_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	int ret;
	int err;
	int addr;
	char tmp[4] = {0x20, 0x00, 0x01, 0x01};
	int addrcnt;
	struct pn547_i2c_platform_data *platform_data;
	/*struct pn547_dev *pn547_dev;*/

#ifdef FEATURE_SEC_NFC_TEST
	struct class * nfc_class;

	nfc_class = class_create(THIS_MODULE, "nfc_test");
	if (IS_ERR(&nfc_class)) 
	{

		pr_err("NFC: failed to create nfc class\n");
	}
	else
	{
		ret = class_create_file(nfc_class, &class_attr_test);
		if (ret)
		    pr_err("NFC: failed to create attr_test\n");
	}
#endif //FEATURE_SEC_NFC_TEST

	pr_info("%s entered\n", __func__);

	if (client->dev.of_node) {
		platform_data = devm_kzalloc(&client->dev,
			sizeof(struct pn547_i2c_platform_data), GFP_KERNEL);
		if (!platform_data) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		err = pn547_parse_dt(&client->dev, platform_data);
		if (err)
			return err;
	} else {
		platform_data = client->dev.platform_data;
	}

	if (platform_data == NULL) {
		pr_err("%s : nfc probe fail\n", __func__);
		return -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s : need I2C_FUNC_I2C\n", __func__);
		return -ENODEV;
	}

	ret = gpio_request(platform_data->irq_gpio, "nfc_int");
	if (ret)
		return -ENODEV;
	ret = gpio_request(platform_data->ven_gpio, "nfc_ven");
	if (ret)
		goto err_ven;
	ret = gpio_request(platform_data->firm_gpio, "nfc_firm");
	if (ret)
		goto err_firm;
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	ret = gpio_request(platform_data->ese_pwr_req, "ese_pwr");
	if (ret)
		goto err_ese;
#endif
#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
	ret = gpio_request(platform_data->clk_req_gpio, "nfc_clk_req");
	if (ret)
		goto err_clk_req;
#endif
#ifdef CONFIG_NFC_PM_BBCLK2
	ret = gpio_request(platform_data->clk_req_gpio, "nfc_clk_req");
	if (ret)
		goto err_clk_req;
#endif

	pn547_dev = kzalloc(sizeof(*pn547_dev), GFP_KERNEL);
	if (pn547_dev == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	if (of_get_property(client->dev.of_node, "pn547,ldo_control", NULL)) {
		ret = pn547_regulator_onoff(client, NFC_I2C_LDO_ON);
		if (ret < 0)
			pr_err("%s pn547 regulator_on fail err = %d\n",
					__func__, ret);
		usleep_range(1000, 1100);
	} else {
		ret = of_get_named_gpio(client->dev.of_node, "pn547,pvdd-gpio", 0);
		if (ret < 0) {
			pr_err("%s : pvdd-gpio is not set", __func__);
		} else
			pn547_dev->pvdd = ret;

		ret = gpio_request(pn547_dev->pvdd, "pvdd-gpio");
		if (ret) {
			dev_err(&client->dev, "%s failed to get gpio pvdd-gpio\n", __func__);
			gpio_free(pn547_dev->pvdd);
			goto err_pvdd;
		}
		gpio_direction_output(pn547_dev->pvdd, 1);
		pr_info("%s pvdd-gpio:%d",__func__, pn547_dev->pvdd);
	}
	
#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
	pn547_dev->nfc_clock = msm_xo_get(MSM_XO_TCXO_A1, "nfc");
	if (IS_ERR(pn547_dev->nfc_clock)) {
		ret = PTR_ERR(pn547_dev->nfc_clock);
		printk(KERN_ERR "%s: Couldn't get TCXO_A1 vote for NFC (%d)\n",
					__func__, ret);
		ret = -ENODEV;
		goto err_get_clock;
	}
	pn547_dev->clock_state = false;
#endif
#ifdef CONFIG_NFC_PM_BBCLK2
	pn547_dev->nfc_clock = clk_get(&client->dev, "nfc_clock");
	if (IS_ERR(pn547_dev->nfc_clock)) {
		ret = PTR_ERR(pn547_dev->nfc_clock);
		printk(KERN_ERR "%s: Couldn't get D1 (%d)\n",
					__func__, ret);
	} else {
		if (clk_prepare_enable(pn547_dev->nfc_clock))
			printk(KERN_ERR "%s: Couldn't prepare D1\n",
					__func__);
	}
#endif
#if defined(CONFIG_NFC_PN547_CLK_BBCLK2)
#if defined(CONFIG_NFC_I2C_OVERWRITE) || defined(CONFIG_NFC_PN547_8084_USE_BBCLK2)
	pn547_dev->nfc_clk = clk_get(NULL, "nfc_clk");
#else
	pn547_dev->nfc_clk = clk_get(&client->dev, "nfc_clk");
#endif
	if (IS_ERR(pn547_dev->nfc_clk)) {
		ret = PTR_ERR(pn547_dev->nfc_clk);
		printk(KERN_ERR "%s: Couldn't get D1 (%d)\n",
					__func__, ret);
	} else {
		if (clk_prepare_enable(pn547_dev->nfc_clk))
			printk(KERN_ERR "%s: Couldn't prepare D1\n",
					__func__);
	}
#endif
#if defined(CONFIG_GPIO_PCAL6416A)
	client->irq = gpio_to_irq(platform_data->irq_gpio);
#endif
	pr_info("%s : IRQ num %d\n", __func__, client->irq);

	pn547_dev->irq_gpio = platform_data->irq_gpio;
	pn547_dev->ven_gpio = platform_data->ven_gpio;
	pn547_dev->firm_gpio = platform_data->firm_gpio;
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	pn547_dev->ese_pwr_req = platform_data->ese_pwr_req;
#endif
	pn547_dev->conf_gpio = platform_data->conf_gpio;
#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
	pn547_dev->clk_req_gpio = platform_data->clk_req_gpio;
	pn547_dev->clk_req_irq = platform_data->clk_req_irq;
#endif
#ifdef CONFIG_NFC_PM_BBCLK2
	pn547_dev->clk_req_gpio = platform_data->clk_req_gpio;
#endif
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	pn547_dev->p61_current_state = P61_STATE_IDLE;
	pn547_dev->nfc_ven_enabled = false;
	pn547_dev->spi_ven_enabled = false;
#endif
	pn547_dev->client = client;

	/* init mutex and queues */
	init_waitqueue_head(&pn547_dev->read_wq);
	mutex_init(&pn547_dev->read_mutex);

#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	sema_init(&ese_access_sema, 1);
	mutex_init(&pn547_dev->p61_state_mutex);
#endif

	pn547_dev->pn547_device.minor = MISC_DYNAMIC_MINOR;
#ifdef CONFIG_NFC_PN547
	pn547_dev->pn547_device.name = "pn547";
#else
	pn547_dev->pn547_device.name = "pn544";
#endif
	pn547_dev->pn547_device.fops = &pn547_dev_fops;

	ret = misc_register(&pn547_dev->pn547_device);
	if (ret) {
		pr_err("%s : misc_register failed\n", __FILE__);
		goto err_misc_register;
	}

	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */
	pr_info("%s : requesting IRQ %d\n", __func__, client->irq);
	gpio_direction_input(pn547_dev->irq_gpio);
	gpio_direction_output(pn547_dev->ven_gpio, 0);
	gpio_direction_output(pn547_dev->firm_gpio, 0);
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	gpio_direction_output(pn547_dev->ese_pwr_req, 0);
#endif
#if defined(CONFIG_NFC_PN547_CLOCK_REQUEST) || defined(CONFIG_NFC_PM_BBCLK2)
	gpio_direction_input(pn547_dev->clk_req_gpio);
#endif

	i2c_set_clientdata(client, pn547_dev);
	wake_lock_init(&pn547_dev->nfc_wake_lock,
			WAKE_LOCK_SUSPEND, "nfc_wake_lock");
#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
	pn547_dev->wq_clock = create_singlethread_workqueue("nfc_wq");
	if (!pn547_dev->wq_clock) {
		ret = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}
	INIT_WORK(&pn547_dev->work_nfc_clock, nfc_work_func_clock);
#endif
	ret = request_irq(client->irq, pn547_dev_irq_handler,
			  IRQF_TRIGGER_RISING, "pn547", pn547_dev);
	if (ret) {
		dev_err(&client->dev, "request_irq failed\n");
		goto err_request_irq_failed;
	}
	disable_irq_nosync(pn547_dev->client->irq);
	atomic_set(&pn547_dev->irq_enabled, 0);

#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
	ret = request_irq(pn547_dev->clk_req_irq, pn547_dev_clk_req_irq_handler,
		IRQF_SHARED | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
			, "pn547_clk_req", pn547_dev);
	if (ret) {
		dev_err(&client->dev, "request_irq(clk_req) failed\n");
		goto err_request_irq_failed;
	}

	enable_irq_wake(pn547_dev->clk_req_irq);
#endif

	gpio_set_value(pn547_dev->ven_gpio, 1);
	gpio_set_value(pn547_dev->firm_gpio, 1); /* add firmware pin */
	usleep_range(4900, 5000);
	gpio_set_value(pn547_dev->ven_gpio, 0);
	usleep_range(4900, 5000);
	gpio_set_value(pn547_dev->ven_gpio, 1);
	usleep_range(4900, 5000);

	for (addr = 0x2B; addr > 0x27; addr--) {
		client->addr = addr;
		addrcnt = 2;

		do {
			ret = i2c_master_send(client, tmp, 4);
			if (ret > 0) {
				pr_info("%s : i2c addr=0x%X\n",
					__func__, client->addr);
				break;
			}
		} while (addrcnt--);

		if (ret > 0)
			break;
	}

	if(ret <= 0)
		client->addr = 0x2B;

	gpio_set_value(pn547_dev->ven_gpio, 0);
	gpio_set_value(pn547_dev->firm_gpio, 0); /* add */

	if (ret < 0)
		pr_err("%s : fail to get i2c addr\n", __func__);
		/* goto err_request_irq_failed; */
	else
		pr_info("%s : success\n", __func__);
	return 0;

err_request_irq_failed:
#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
err_create_workqueue:
#endif
	misc_deregister(&pn547_dev->pn547_device);
	wake_lock_destroy(&pn547_dev->nfc_wake_lock);
err_misc_register:
	mutex_destroy(&pn547_dev->read_mutex);
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	mutex_destroy(&pn547_dev->p61_state_mutex);
#endif
#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
	msm_xo_put(pn547_dev->nfc_clock);
err_get_clock:
#endif
err_pvdd:
	kfree(pn547_dev);
err_exit:
#if defined(CONFIG_NFC_PN547_CLOCK_REQUEST) || defined(CONFIG_NFC_PM_BBCLK2)
	gpio_free(platform_data->clk_req_gpio);
err_clk_req:
#endif
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	gpio_free(platform_data->ese_pwr_req);
err_ese:
#endif
	gpio_free(platform_data->firm_gpio);
err_firm:
	gpio_free(platform_data->ven_gpio);
err_ven:
	gpio_free(platform_data->irq_gpio);
	pr_err("[pn547] pn547_probe fail!\n");
	return ret;
}

static int pn547_remove(struct i2c_client *client)
{
	struct pn547_dev *pn547_dev;

	pn547_dev = i2c_get_clientdata(client);
#ifdef CONFIG_NFC_PM_BBCLK2
	if(pn547_dev->nfc_clock)
		clk_unprepare(pn547_dev->nfc_clock);
#endif
#if defined(CONFIG_NFC_PN547_CLK_BBCLK2)
	if (pn547_dev->nfc_clk)
		clk_unprepare(pn547_dev->nfc_clk);
#endif
	wake_lock_destroy(&pn547_dev->nfc_wake_lock);
	free_irq(client->irq, pn547_dev);
	misc_deregister(&pn547_dev->pn547_device);
	mutex_destroy(&pn547_dev->read_mutex);
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	mutex_destroy(&pn547_dev->p61_state_mutex);
#endif
	gpio_free(pn547_dev->irq_gpio);
	gpio_free(pn547_dev->ven_gpio);
	gpio_free(pn547_dev->firm_gpio);
#ifdef CONFIG_NFC_PM_BBCLK2
	gpio_free(pn547_dev->clk_req_gpio);
#endif
#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
	gpio_free(pn547_dev->clk_req_gpio);
	msm_xo_put(pn547_dev->nfc_clock);
#endif
#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
	pn547_dev->p61_current_state = P61_STATE_INVALID;
	pn547_dev->nfc_ven_enabled = false;
	pn547_dev->spi_ven_enabled = false;
#endif
	kfree(pn547_dev);

	return 0;
}

static const struct i2c_device_id pn547_id[] = {
	{"pn547", 0},
	{}
};

#ifdef CONFIG_OF
static struct of_device_id nfc_match_table[] = {
	{ .compatible = "pn547",},
	{},
};
#else
#define nfc_match_table NULL
#endif

static struct i2c_driver pn547_driver = {
	.id_table = pn547_id,
	.probe = pn547_probe,
	.remove = pn547_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "pn547",
		.of_match_table = nfc_match_table,
	},
};

/*
 * module load/unload record keeping
 */

/*extern unsigned int system_rev;*/
static int __init pn547_dev_init(void)
{
	pr_info("Loading pn547 driver\n");

	if(poweroff_charging)
		return 0;
	else
		return i2c_add_driver(&pn547_driver);

}

module_init(pn547_dev_init);

static void __exit pn547_dev_exit(void)
{
	pr_info("Unloading pn547 driver\n");
	i2c_del_driver(&pn547_driver);
}

module_exit(pn547_dev_exit);

MODULE_AUTHOR("Sylvain Fonteneau");
MODULE_DESCRIPTION("NFC PN547 driver");
MODULE_LICENSE("GPL");
