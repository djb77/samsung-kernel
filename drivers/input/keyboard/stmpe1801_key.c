#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/input/matrix_keypad.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/errno.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/wakelock.h>

#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <linux/sec_class.h>

#include <linux/input/stmpe1801_key.h>

#define DD_VER	"2.4"

#define DBG_PRN	// define for debugging printk
//#define STMPE1801_TRIGGER_TYPE_FALLING // define for INT trigger
#define STMPE1801_TMR_INTERVAL	10L

#define SUPPORTED_KEYPAD_LED
#define SUPPORTED_GPIO_SYSFS
#define USE_SUSPEND_LATE

/*
 * Definitions & globals.
 */
#define STMPE1801_MAXGPIO			18
#define STMPE1801_KEY_FIFO_LENGTH		10

#define STMPE1801_REG_SYS_CHIPID		0x00
#define STMPE1801_REG_SYS_CTRL			0x02

#define STMPE1801_REG_INT_CTRL_LOW		0x04
#define STMPE1801_REG_INT_MASK_LOW		0x06
#define STMPE1801_REG_INT_STA_LOW		0x08

#define STMPE1801_REG_INT_GPIO_MASK_LOW		0x0A
#define STMPE1801_REG_INT_GPIO_STA_LOW		0x0D

#define STMPE1801_REG_GPIO_SET_yyy		0x10
#define STMPE1801_REG_GPIO_CLR_yyy		0x13
#define STMPE1801_REG_GPIO_MP_yyy		0x16
#define STMPE1801_REG_GPIO_SET_DIR_yyy		0x19
#define STMPE1801_REG_GPIO_RE_yyy		0x1C
#define STMPE1801_REG_GPIO_FE_yyy		0x1F
#define STMPE1801_REG_GPIO_PULL_UP_yyy		0x22

#define STMPE1801_REG_KPC_ROW			0x30
#define STMPE1801_REG_KPC_COL_LOW		0x31
#define STMPE1801_REG_KPC_CTRL_LOW		0x33
#define STMPE1801_REG_KPC_CTRL_MID		0x34
#define STMPE1801_REG_KPC_CTRL_HIGH		0x35
#define STMPE1801_REG_KPC_CMD			0x36
#define STMPE1801_REG_KPC_DATA_BYTE0		0x3A


#define STMPE1801_KPC_ROW_SHIFT			3 // can be 3

#define STMPE1801_KPC_DATA_UP			(0x80)
#define STMPE1801_KPC_DATA_COL			(0x78)
#define STMPE1801_KPC_DATA_ROW			(0x07)
#define STMPE1801_KPC_DATA_NOKEY		(0x0f)

union stmpe1801_kpc_data {
	uint64_t	value;
	uint8_t		array[8];
};

struct stmpe1801_dev {
	int					dev_irq;
	struct i2c_client			*client;
	struct input_dev			*input_dev;
	struct delayed_work			worker;
	struct mutex				dev_lock;
	struct wake_lock			stmpe_wake_lock;
	atomic_t				dev_enabled;
	atomic_t				dev_init;
	int					dev_resume_state;
	struct device				*sec_keypad;

	u8					regs_sys_int_init[4];
	u8					regs_kpc_init[8];
	u8					regs_pullup_init[3];

	struct matrix_keymap_data		*keymap_data;
	unsigned short				*keycode;
	int					keycode_entries;

	bool					probe_done;

	struct stmpe1801_devicetree_data	*dtdata;
	int					irq_gpio;
	int					sda_gpio;
	int					scl_gpio;
	struct pinctrl				*pinctrl;
	int					key_state[5];
};

struct stmpe1801_devicetree_data {
	int gpio_int;
	int gpio_sda;
	int gpio_scl;
	int gpio_rst;
	int gpio_tkey_led_en;
	int block_type;
	int debounce;
	int scan_cnt;
	int repeat;
	int num_row;
	int num_column;
#ifdef USE_SUSPEND_LATE
	int sleep_state;
#endif
	struct regulator *vdd_vreg;
	struct regulator *reset_vreg;
};

static struct stmpe1801_dev *copy_device_data;

#ifdef SUPPORT_STM_INT
// # Support GPIO : 6, 7, 14, 15, 16, 17 and Only use #6 for FG on NOVEL
struct stmpe1801_int_data stmpe1801_int_data[] = {
	{
		.gpio_num = 6,
		.irq_enable = 0,
		.irq_type = 0,
		.offset_addr = 0,
		.offset_bit = 6,
		.device_name = "",
	},
#if 0
	{
		.gpio_num = 5,
		.irq_enable = 0,
		.irq_type = 0,
		.offset_addr = 0,
		.offset_bit = 5,
		.device_name = "",
	},
	{
		.gpio_num = 7,
		.irq_enable = 0,
		.irq_type = 0,
		.offset_addr = 0,
		.offset_bit = 7,
		.device_name = "",
	},
	{
		.gpio_num = 14,
		.irq_enable = 0,
		.irq_type = 0,
		.offset_addr = 1,
		.offset_bit = 6,
		.device_name = "",
	},
	{
		.gpio_num = 15,
		.irq_enable = 0,
		.irq_type = 0,
		.offset_addr = 1,
		.offset_bit = 7,
		.device_name = "",
	},
	{
		.gpio_num = 16,
		.irq_enable = 0,
		.irq_type = 0,
		.offset_addr = 2,
		.offset_bit = 0,
		.device_name = "",
	},
	{
		.gpio_num = 17,
		.irq_enable = 0,
		.irq_type = 0,
		.offset_addr = 2,
		.offset_bit = 1,
		.device_name = "",
	},
#endif
};
#endif

static int __stmpe1801_get_keycode_entries(struct stmpe1801_dev *stmpe1801, u32 mask_row, u32 mask_col)
{
	int col, row;
	int i, j, comp;

	row = mask_row & 0xff;
	col = mask_col & 0x3ff;

	for (i = 8; i > 0; i--) {
		comp = 1 << (i - 1);
		input_info(true, &stmpe1801->client->dev, "row: %d, comp: %d\n", row, comp);
		if (row & comp) break;
	}

	for (j = 10; j > 0; j--) {
		comp = 1 << (j - 1);
		input_info(true, &stmpe1801->client->dev, "col: %d, comp: %d\n", col, comp);
		if (col & comp) break;
	}
#ifdef DBG_PRN
	input_info(true, &stmpe1801->client->dev, "row: %d, col: %d\n", i, j);
#endif

	return (MATRIX_SCAN_CODE(i - 1, j - 1, STMPE1801_KPC_ROW_SHIFT)+1);
}


static int __i2c_block_read(struct i2c_client *client, u8 regaddr, u8 length, u8 *values)
{
	int ret;
	int retry = 3;
	struct i2c_msg	msgs[] = {
		{
			.addr = client->addr,
			.flags = client->flags,
			.len = 1,
			.buf = &regaddr,
		},
		{
			.addr = client->addr,
			.flags = client->flags | I2C_M_RD,
			.len = length,
			.buf = values,
		},
	};

	while (retry--) {
		ret = i2c_transfer(client->adapter, msgs, 2);

		if (ret != 2) {
			input_err(true, &client->dev, "failed to read reg %#x: %d\n", regaddr, ret);
			ret = -EIO;
				usleep_range(10000, 10000);
				continue;
		}
		break;
	}

	return ret;
}

static int __i2c_block_write(struct i2c_client *client, u8 regaddr, u8 length, const u8 *values)
{
	int ret;
	int retry = 3;
	unsigned char msgbuf0[I2C_SMBUS_BLOCK_MAX+1];
	struct i2c_msg	msgs[] = {
		{
			.addr = client->addr,
			.flags = client->flags,
			.len = 1 + length,
			.buf = msgbuf0,
		},
	};

	if (length > I2C_SMBUS_BLOCK_MAX)
		return -E2BIG;

	memcpy(msgbuf0 + 1, values, length * sizeof(u8));
	msgbuf0[0] = regaddr;

	while (retry--) {
		ret = i2c_transfer(client->adapter, msgs, 1);

		if (ret != 1) {
			input_err(true, &client->dev, "failed to write reg %#x: %d\n", regaddr, ret);
			ret = -EIO;
				usleep_range(10000, 10000);
				continue;
		}
		break;
	}

	return ret;
}


static int __i2c_reg_read(struct i2c_client *client, u8 regaddr)
{
	int ret;
	u8 buf[4];

	ret = __i2c_block_read(client, regaddr, 1, buf);
	if (ret < 0)
		return ret;

	return buf[0];
}

static int __i2c_reg_write(struct i2c_client *client, u8 regaddr, u8 val)
{
	int ret;

	ret = __i2c_block_write(client, regaddr, 1, &val);
	if (ret < 0)
		return ret;

	return ret;
}

static int __i2c_set_bits(struct i2c_client *client, u8 regaddr, u8 offset, u8 val)
{
	int ret;
	u8 mask;

	ret = __i2c_reg_read(client, regaddr);
	if (ret < 0)
		return ret;

	mask = 1 << offset;

	if (val > 0) {
		ret |= mask;
	}
	else {
		ret &= ~mask;
	}

	return __i2c_reg_write(client, regaddr, ret);
}

static void stmpe1801_regs_init(struct stmpe1801_dev *stmpe1801)
{

#ifdef DBG_PRN
	input_info(true, &stmpe1801->client->dev, "dev lock @ %s\n", __func__);
#endif
	mutex_lock(&stmpe1801->dev_lock);

	// 0: SYS_CTRL
	stmpe1801->regs_sys_int_init[0] = stmpe1801->dtdata->debounce;
	// 1: INT_CTRL
	stmpe1801->regs_sys_int_init[1] =
		STMPE1801_INT_CTRL_ACTIVE_LOW
#ifdef STMPE1801_TRIGGER_TYPE_FALLING
		|STMPE1801_INT_CTRL_TRIG_EDGE
#else
		|STMPE1801_INT_CTRL_TRIG_LEVEL
#endif
		|STMPE1801_INT_CTRL_GLOBAL_ON;
	// 2: INT_EN_MASK
	stmpe1801->regs_sys_int_init[2] = 0;
	if (stmpe1801->dtdata->block_type & STMPE1801_BLOCK_GPIO)
		stmpe1801->regs_sys_int_init[2] |= STMPE1801_INT_MASK_GPIO;
	if (stmpe1801->dtdata->block_type & STMPE1801_BLOCK_KEY)
		stmpe1801->regs_sys_int_init[2] |= STMPE1801_INT_MASK_KEY|STMPE1801_INT_MASK_KEY_FIFO;

	if ((stmpe1801->dtdata->block_type & STMPE1801_BLOCK_KEY) && (stmpe1801->keymap_data != NULL)) {
		// 0 : KPC_ROW
		unsigned int i;
		i = (((1 << stmpe1801->dtdata->num_column) - 1) << 8)| ((1 << stmpe1801->dtdata->num_row) - 1);
		stmpe1801->regs_kpc_init[0] = i & 0xff;	// Row
		// 1 : KPC_COL_LOW
		stmpe1801->regs_kpc_init[1] = (i >> 8) & 0xff; // Col_low
		// 2 : KPC_COL_HIGH
		stmpe1801->regs_kpc_init[2] = (i >> 16) & 0x03; // Col_high
		// Pull-up in unused pin
		i = ~i;
		stmpe1801->regs_pullup_init[0] = i & 0xff;
		stmpe1801->regs_pullup_init[1] = (i >> 8) & 0xff;
		stmpe1801->regs_pullup_init[2] = (i >> 16) & 0x03;
		// 3 : KPC_CTRL_LOW
		i = stmpe1801->dtdata->scan_cnt & 0x0f;
		stmpe1801->regs_kpc_init[3] = (i << STMPE1801_SCAN_CNT_SHIFT) & STMPE1801_SCAN_CNT_MASK;
		// 4 : KPC_CTRL_MID
		stmpe1801->regs_kpc_init[4] = 0x62; // default : 0x62
		// 5 : KPC_CTRL_HIGH
		stmpe1801->regs_kpc_init[5] = (0x40 & ~STMPE1801_SCAN_FREQ_MASK) | STMPE1801_SCAN_FREQ_60HZ;
	}

	if (stmpe1801->dtdata->block_type & STMPE1801_BLOCK_GPIO
		/*&& (stmpe1801->pdata != NULL)*/) {
		// NOP
	}

#ifdef DBG_PRN
	input_info(true, &stmpe1801->client->dev, "dev unlock @ %s\n", __func__);
#endif
	mutex_unlock(&stmpe1801->dev_lock);

	return;
}

static int stmpe1801_hw_init(struct stmpe1801_dev *stmpe1801)
{
	int ret;
	int i=0;
	int stmpe1801_int_data_size = sizeof(stmpe1801_int_data)/sizeof(struct stmpe1801_int_data);

	//Check ID
	ret = __i2c_reg_read(stmpe1801->client, STMPE1801_REG_SYS_CHIPID);
#ifdef DBG_PRN
	input_info(true, &stmpe1801->client->dev, "Chip ID = %x %x\n" ,
		ret, __i2c_reg_read(stmpe1801->client, STMPE1801_REG_SYS_CHIPID+1));
#endif
	if (ret != 0xc1)  goto err_hw_init;

	//soft reset & set debounce time
	ret = __i2c_reg_write(stmpe1801->client, STMPE1801_REG_SYS_CTRL,
		0x80 | stmpe1801->regs_sys_int_init[0]);
	if (ret < 0) goto err_hw_init;
	mdelay(20);	// waiting reset

	// INT CTRL - It is erased if it is not need.
	ret = __i2c_reg_write(stmpe1801->client, STMPE1801_REG_INT_CTRL_LOW,
		stmpe1801->regs_sys_int_init[1]);
	if (ret < 0) goto err_hw_init;


	// INT EN Mask INT_EN_MASK_LOW Register
	ret = __i2c_reg_write(stmpe1801->client, STMPE1801_REG_INT_MASK_LOW,
		stmpe1801->regs_sys_int_init[2]);
	if (ret < 0) goto err_hw_init;

	if (stmpe1801->dtdata->block_type & STMPE1801_BLOCK_KEY) {
		ret = __i2c_block_write(stmpe1801->client, STMPE1801_REG_KPC_ROW, 6, stmpe1801->regs_kpc_init);
		if (ret < 0) goto err_hw_init;
		ret = __i2c_block_write(stmpe1801->client, STMPE1801_REG_GPIO_PULL_UP_yyy, 3, stmpe1801->regs_pullup_init);
		if (ret < 0) goto err_hw_init;
#ifdef DBG_PRN
		input_info(true, &stmpe1801->client->dev, "Keypad setting done.\n");
#endif
	}

	if (stmpe1801->dtdata->block_type & STMPE1801_BLOCK_GPIO) {
		for (i = 0 ; i < stmpe1801_int_data_size ; i++){
			/* Mask using GPIO pin */
			ret = __i2c_set_bits(stmpe1801->client, STMPE1801_REG_INT_GPIO_MASK_LOW + stmpe1801_int_data[i].offset_addr,
											stmpe1801_int_data[i].offset_bit, 1);
			if (ret < 0)
				input_err(true, &stmpe1801->client->dev, "%s : STMPE1801_REG_INT_GPIO_MASK_LOW ERROR.\n", __func__);

			/* Set GPIO INPUT */
			ret = __i2c_set_bits(stmpe1801->client, STMPE1801_REG_GPIO_SET_DIR_yyy + stmpe1801_int_data[i].offset_addr,
											stmpe1801_int_data[i].offset_bit, 0);
			if (ret < 0)
				input_err(true, &stmpe1801->client->dev, "%s : STMPE1801_REG_GPIO_SET_DIR_yyy ERROR.\n", __func__);

			ret = __i2c_set_bits(stmpe1801->client, STMPE1801_REG_GPIO_FE_yyy + stmpe1801_int_data[i].offset_addr,
											stmpe1801_int_data[i].offset_bit, 1);
			if (ret < 0)
				input_err(true, &stmpe1801->client->dev, "%s : STMPE1801_REG_GPIO_FE_yyy ERROR.\n", __func__);

#ifdef DBG_PRN
			input_info(true, &stmpe1801->client->dev, "GPIO [%d] setting done.\n", stmpe1801_int_data[i].gpio_num);
#endif
		}
	}


err_hw_init:

	return ret;
}

static int stmpe1801_dev_power_off(struct stmpe1801_dev *stmpe1801)
{
#ifdef DBG_PRN
	input_info(true, &stmpe1801->client->dev, "%s\n", __func__);
#endif

	if (atomic_read(&stmpe1801->dev_init) > 0) {
		atomic_set(&stmpe1801->dev_init, 0);
	}

#ifdef DBG_PRN
	input_info(true, &stmpe1801->client->dev, "return ok @ %s\n", __func__);
#endif

	return 0;
}


static int stmpe1801_dev_power_on(struct stmpe1801_dev *stmpe1801)
{
	int ret = -EINVAL;

#ifdef DBG_PRN
	input_info(true, &stmpe1801->client->dev, "%s\n", __func__);
#endif

	if (!atomic_read(&stmpe1801->dev_init)) {
		ret = stmpe1801_hw_init(stmpe1801);
		if (ret < 0) {
			stmpe1801_dev_power_off(stmpe1801);
			return ret;
		}
		atomic_set(&stmpe1801->dev_init, 1);
	}

	return 0;
}

static int stmpe1801_dev_kpc_disable(struct stmpe1801_dev *stmpe1801)
{
	int ret = -EINVAL;

	if (atomic_cmpxchg(&stmpe1801->dev_enabled, 1, 0)) {
#ifdef DBG_PRN
		input_info(true, &stmpe1801->client->dev, "dev lock @ %s\n", __func__);
#endif
		mutex_lock(&stmpe1801->dev_lock);

		if (atomic_read(&stmpe1801->dev_init) > 0) {
			if (stmpe1801->dtdata->gpio_int >= 0)
				disable_irq_nosync(stmpe1801->dev_irq);
			else
				cancel_delayed_work(&stmpe1801->worker);
		}

		ret = stmpe1801_dev_power_off(stmpe1801);
		if (ret < 0) goto err_dis;

		// stop scanning
		ret = __i2c_set_bits(stmpe1801->client, STMPE1801_REG_KPC_CMD, 0, 0);
		if (ret < 0) goto err_dis;

#ifdef DBG_PRN
		input_info(true, &stmpe1801->client->dev, "dev unlock @ %s\n", __func__);
#endif
		mutex_unlock(&stmpe1801->dev_lock);
	}

	return 0;

err_dis:
#ifdef DBG_PRN
	input_err(true, &stmpe1801->client->dev, "err: dev unlock @ %s\n", __func__);
#endif
	mutex_unlock(&stmpe1801->dev_lock);

	return ret;
}


static int stmpe1801_dev_kpc_enable(struct stmpe1801_dev *stmpe1801)
{
	int ret = -EINVAL;

	if (!atomic_cmpxchg(&stmpe1801->dev_enabled, 0, 1)) {
#ifdef DBG_PRN
		input_info(true, &stmpe1801->client->dev, "dev lock @ %s\n", __func__);
#endif
		mutex_lock(&stmpe1801->dev_lock);

		ret = stmpe1801_dev_power_on(stmpe1801);
		if (ret < 0) goto err_en;

		// start scanning
		ret = __i2c_set_bits(stmpe1801->client, STMPE1801_REG_KPC_CMD, 0, 1);
		if (ret < 0) goto err_en;

		if (atomic_read(&stmpe1801->dev_init) > 0) {
			if (stmpe1801->dtdata->gpio_int >= 0)
				enable_irq(stmpe1801->dev_irq);
			else
				schedule_delayed_work(&stmpe1801->worker, STMPE1801_TMR_INTERVAL);
		}

#ifdef DBG_PRN
		input_info(true, &stmpe1801->client->dev, "dev unlock @ %s\n", __func__);
#endif
		mutex_unlock(&stmpe1801->dev_lock);
	}
	return 0;

err_en:
	atomic_set(&stmpe1801->dev_enabled, 0);
#ifdef DBG_PRN
	input_err(true, &stmpe1801->client->dev, "err: dev unlock @ %s\n", __func__);
#endif
	mutex_unlock(&stmpe1801->dev_lock);

	return ret;
}

int stmpe1801_dev_kpc_input_open(struct input_dev *dev)
{
	struct stmpe1801_dev *stmpe1801 = input_get_drvdata(dev);

	return stmpe1801_dev_kpc_enable(stmpe1801);
}

void stmpe1801_dev_kpc_input_close(struct input_dev *dev)
{
	struct stmpe1801_dev *stmpe1801 = input_get_drvdata(dev);

	stmpe1801_dev_kpc_disable(stmpe1801);

	return;
}

static void stmpe1801_dev_int_proc(struct stmpe1801_dev *stmpe1801)
{
	int ret, i, j;
	int data, col, row, code;
	bool up;
	union stmpe1801_kpc_data key_values;
	int stmpe1801_int_data_size = sizeof(stmpe1801_int_data)/sizeof(struct stmpe1801_int_data);

	mutex_lock(&stmpe1801->dev_lock);
	wake_lock(&stmpe1801->stmpe_wake_lock);
	// disable chip global int
	__i2c_set_bits(stmpe1801->client, STMPE1801_REG_INT_CTRL_LOW, 0, 0);

	// get int src
	ret = __i2c_reg_read(stmpe1801->client, STMPE1801_REG_INT_STA_LOW);
	if (ret < 0) goto out_proc;

	input_dbg(true, &stmpe1801->client->dev,
					"%s Read STMPE1801_REG_INT_STA_LOW[%d]\n", __func__, ret);

#ifdef SUPPORT_STM_INT
	if (stmpe1801->dtdata->block_type & STMPE1801_BLOCK_GPIO) {	// GPIO mode
		if (ret & STMPE1801_INT_MASK_GPIO) {	// GPIO mode
			ret = __i2c_reg_read(stmpe1801->client, STMPE1801_REG_INT_GPIO_STA_LOW);
			input_info(true, &stmpe1801->client->dev,
							"%s Read STMPE1801_REG_INT_GPIO_STA_LOW[0x%X]\n", __func__, ret);

			for (i=0 ; i < stmpe1801_int_data_size ; i++){
				if ((1 << stmpe1801_int_data[i].offset_bit) & ret) {
					input_info(true, &stmpe1801->client->dev, "%s : gpio[%d] %s\n",
							__func__, stmpe1801_int_data[i].gpio_num,
							stmpe1801_int_data[i].stmpe_irq_func == NULL ? "Find Func() but not init." : "Run Func()");
					if (stmpe1801_int_data[i].stmpe_irq_func != NULL)
						stmpe1801_int_data[i].stmpe_irq_func(stmpe1801_int_data[i].dev_id);
				}
			}
		}
	}
#endif

	if (stmpe1801->dtdata->block_type & STMPE1801_BLOCK_KEY) {	// KEYPAD mode
		if((ret & (STMPE1801_INT_MASK_KEY|STMPE1801_INT_MASK_KEY_FIFO)) > 0) { // INT
			for (i = 0; i < STMPE1801_KEY_FIFO_LENGTH; i++) {
				ret = __i2c_block_read(stmpe1801->client, STMPE1801_REG_KPC_DATA_BYTE0, 5, key_values.array);
				if (ret > 0) {
					if (key_values.value != 0xffff8f8f8LL) {
						for (j = 0; j < 3; j++) {
							data = key_values.array[j];
							col = (data & STMPE1801_KPC_DATA_COL) >> 3;
							row = data & STMPE1801_KPC_DATA_ROW;
							code = MATRIX_SCAN_CODE(row, col, STMPE1801_KPC_ROW_SHIFT);
							up = data & STMPE1801_KPC_DATA_UP ? 1 : 0;

							if (col == STMPE1801_KPC_DATA_NOKEY)
								continue;

							if (up) {	/* Release */
								stmpe1801->key_state[row] &= ~(1 << col);
							} else {	/* Press */
								stmpe1801->key_state[row] |= (1 << col);
							}
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
							input_info(true, &stmpe1801->client->dev, "%s code(0x%X|%d) R:C(%d:%d)\n",
									!up ? "P" : "R", stmpe1801->keycode[code], stmpe1801->keycode[code], row, col);
#else
							input_info(true, &stmpe1801->client->dev, "%s (%d:%d)\n",
									!up ? "P" : "R", row, col);
#endif
							input_event(stmpe1801->input_dev, EV_MSC, MSC_SCAN, code);
							input_report_key(stmpe1801->input_dev, stmpe1801->keycode[code], !up);
							input_sync(stmpe1801->input_dev);
						}
					}
				}
			}
		}
	}
#ifdef DBG_PRN
	input_dbg(true, &stmpe1801->client->dev, "%s -- dev_int_proc --\n",__func__);
#endif

out_proc:
	// enable chip global int
	__i2c_set_bits(stmpe1801->client, STMPE1801_REG_INT_CTRL_LOW, 0, 1);

	wake_unlock(&stmpe1801->stmpe_wake_lock);
	mutex_unlock(&stmpe1801->dev_lock);

}

static irqreturn_t stmpe1801_dev_isr(int irq, void *dev_id)
{
	struct stmpe1801_dev *stmpe1801 = (struct stmpe1801_dev *)dev_id;
#ifdef USE_SUSPEND_LATE
	int retry = 0;

	while(stmpe1801->dtdata->sleep_state == true){
		input_info(true, &stmpe1801->client->dev, "%s sleep_state retry=%d\n", __func__, retry);
		usleep_range(10 * 1000, 10 * 1000);
		if (++retry > 5)
			return IRQ_HANDLED;
	}
#endif
	stmpe1801_dev_int_proc(stmpe1801);

	return IRQ_HANDLED;
}

static void stmpe1801_dev_worker(struct work_struct *work)
{
	struct stmpe1801_dev *stmpe1801 = container_of((struct delayed_work *)work, struct stmpe1801_dev, worker);

	stmpe1801_dev_int_proc(stmpe1801);

	schedule_delayed_work(&stmpe1801->worker, STMPE1801_TMR_INTERVAL);
}

#ifdef CONFIG_OF
static int stmpe1801_parse_dt(struct device *dev,
			struct stmpe1801_dev *device_data)
{
	struct device_node *np = dev->of_node;
	struct matrix_keymap_data *keymap_data;
	int ret, keymap_len, i;
	u32 *keymap, temp;
	const __be32 *map;

	device_data->dtdata->gpio_int = of_get_named_gpio(np, "stmpe,irq_gpio", 0);
	if(device_data->dtdata->gpio_int < 0){
		input_err(true, dev, "unable to get gpio_int\n");
		return device_data->dtdata->gpio_int;
	}

	device_data->dtdata->gpio_sda = of_get_named_gpio(np, "stmpe,sda_gpio", 0);
	device_data->dtdata->gpio_scl = of_get_named_gpio(np, "stmpe,scl_gpio", 0);
	device_data->dtdata->gpio_rst = of_get_named_gpio(np, "stmpe,rst_gpio", 0);

	ret = of_property_read_u32(np, "stmpe,block_type", &temp);
	if(ret){
		input_err(true, dev, "unable to get block_type\n");
		return ret;
	}
	device_data->dtdata->block_type = temp;

	ret = of_property_read_u32(np, "stmpe,debounce", &temp);
	if(ret){
		input_err(true, dev, "unable to get debounce\n");
		return ret;
	}
	device_data->dtdata->debounce = temp;

	ret = of_property_read_u32(np, "stmpe,scan_cnt", &temp);
	if(ret){
		input_err(true, dev, "unable to get scan_cnt\n");
		return ret;
	}
	device_data->dtdata->scan_cnt = temp;

	ret = of_property_read_u32(np, "stmpe,repeat", &temp);
	if(ret){
		input_err(true, dev, "unable to get repeat\n");
		return ret;
	}
	device_data->dtdata->repeat = temp;

	device_data->dtdata->gpio_tkey_led_en = of_get_named_gpio(np, "stmpe,led_en-gpio", 0);
	if(device_data->dtdata->gpio_tkey_led_en < 0){
		input_err(true, dev, "unable to get gpio_tkey_led_en...ignoring\n");
	}

	ret = of_property_read_u32(np, "keypad,num-rows", &temp);
	if(ret){
		input_err(true, dev, "unable to get num-rows\n");
		return ret;
	}
	device_data->dtdata->num_row = temp;

	ret = of_property_read_u32(np, "keypad,num-columns", &temp);
	if(ret){
		input_err(true, dev, "unable to get num-columns\n");
		return ret;
	}
	device_data->dtdata->num_column = temp;

	map = of_get_property(np, "linux,keymap", &keymap_len);

	if (!map) {
		input_err(true, dev, "Keymap not specified\n");
		return -EINVAL;
	}

	keymap_data = devm_kzalloc(dev,
					sizeof(*keymap_data), GFP_KERNEL);
	if (!keymap_data) {
		input_err(true, dev, "Unable to allocate memory\n");
		return -ENOMEM;
	}

	keymap_data->keymap_size = keymap_len / sizeof(u32);

	keymap = devm_kzalloc(dev,
		sizeof(uint32_t) * keymap_data->keymap_size, GFP_KERNEL);
	if (!keymap) {
		input_err(true, dev, "could not allocate memory for keymap\n");
		return -ENOMEM;
	}

	for (i = 0; i < keymap_data->keymap_size; i++) {
		unsigned int key = be32_to_cpup(map + i);
		int keycode, row, col;

		row = (key >> 24) & 0xff;
		col = (key >> 16) & 0xff;
		keycode = key & 0xffff;
		keymap[i] = KEY(row, col, keycode);
	}
	keymap_data->keymap = keymap;
	device_data->keymap_data = keymap_data;
#ifdef USE_SUSPEND_LATE
	device_data->dtdata->sleep_state = false;
#endif
	input_info(true, dev, "%s: gpio_int:%d, gpio_led_en:%d, block_type:%d\n",
			__func__, device_data->dtdata->gpio_int, device_data->dtdata->gpio_tkey_led_en,
			device_data->dtdata->block_type);


	return 0;
}
#else
static int abov_parse_dt(struct device *dev,
			struct abov_touchkey_devicetree_data *dtdata)
{
	return -ENODEV;
}
#endif

static int stmpe1801_dev_regulator_on(struct device *dev,
			struct stmpe1801_dev *device_data)
{
	int ret = 0;

	device_data->dtdata->reset_vreg = devm_regulator_get(dev, "reset");
	if (IS_ERR(device_data->dtdata->reset_vreg)) {
		input_err(true, dev, "%s: could not get expander_rst, rc = %ld\n",
			__func__, PTR_ERR(device_data->dtdata->reset_vreg));
		device_data->dtdata->reset_vreg = NULL;
		return -ENODEV;
	}
	ret = regulator_enable(device_data->dtdata->reset_vreg);
	if (ret) {
		input_err(true, dev, "%s: Failed to enable expander_rst: %d\n", __func__, ret);
		return ret;
	}

	device_data->dtdata->vdd_vreg = devm_regulator_get(dev, "vddo");
	if (IS_ERR(device_data->dtdata->vdd_vreg)) {
		input_err(true, dev, "%s: could not get vddo, rc = %ld\n",
			__func__, PTR_ERR(device_data->dtdata->vdd_vreg));
		device_data->dtdata->vdd_vreg = NULL;
		return -ENODEV;
	}
	ret = regulator_enable(device_data->dtdata->vdd_vreg);
	if (ret) {
		input_err(true, dev, "%s: Failed to enable vddo: %d\n", __func__, ret);
		return ret;
	}

	input_err(true, dev, "stmpe1801_reset_power is reset:%s, vddo:%s\n",
		regulator_is_enabled(device_data->dtdata->reset_vreg) ? "on" : "off",
		regulator_is_enabled(device_data->dtdata->vdd_vreg) ? "on" : "off");

	return ret;
}

static ssize_t  sysfs_key_onoff_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stmpe1801_dev *device_data = dev_get_drvdata(dev);
	int state = 0;
	int i;

	for (i = 0; i < 5; i++) {
		state |= device_data->key_state[i];
	}

	input_info(true, &device_data->client->dev,
		"%s: key state:%d\n", __func__, !!state);

	return snprintf(buf, 5, "%d\n", !!state);
}

static DEVICE_ATTR(sec_key_pressed, 0444 , sysfs_key_onoff_show, NULL);

#ifdef SUPPORTED_KEYPAD_LED
static ssize_t key_led_onoff_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct stmpe1801_dev *device_data = dev_get_drvdata(dev);
	int state = gpio_get_value(device_data->dtdata->gpio_tkey_led_en);

	input_info(true, dev, "%s: key_led_%s\n", __func__, state ? "on" : "off");

	return snprintf(buf, 5, "%d\n", state);
}

static ssize_t key_led_onoff(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	int data;
	struct stmpe1801_dev *device_data = dev_get_drvdata(dev);

	sscanf(buf, "%d\n", &data);

	gpio_direction_output(device_data->dtdata->gpio_tkey_led_en, !!data);
	input_err(true, dev, "%s: key_led_%s: %s now\n", __func__,
		(!!data) ? "on" : "off",
		gpio_get_value(device_data->dtdata->gpio_tkey_led_en) ? "on" : "off");

	return size;
}

static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP,
		key_led_onoff_show, key_led_onoff);
#endif

#ifdef SUPPORTED_GPIO_SYSFS
static ssize_t expander_gpio_control(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	int data;

	sscanf(buf, "%d\n", &data);

	if (data) {
		stmpe1801_gpio_enable(1);
		input_err(true, dev, "[KEY] %s expander_gpio %d\n",__func__, data);
	}
	else {
		stmpe1801_gpio_enable(0);
		input_err(true, dev, "[KEY] %s expander_gpio %d\n",__func__, data);
	}

	return size;
}

static DEVICE_ATTR(expander_gpio, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, expander_gpio_control);
#endif

static struct attribute *key_attributes[] = {
	&dev_attr_sec_key_pressed.attr,
#ifdef SUPPORTED_KEYPAD_LED
	&dev_attr_brightness.attr,
#endif
#ifdef SUPPORTED_GPIO_SYSFS
	&dev_attr_expander_gpio.attr,
#endif
	NULL,
};

static struct attribute_group key_attr_group = {
	.attrs = key_attributes,
};

void stmpe1801_gpio_enable(bool en)
{
	struct stmpe1801_dev *data = copy_device_data;
	int ret;
	input_info(true, &data->client->dev, "%s + en:%d\n",__func__, en);

	// NOVEL gpio 5 set for TDMB_FM_SEL
	ret = __i2c_reg_write(data->client, STMPE1801_REG_GPIO_SET_DIR_yyy, 0x20);		//gpio5 0x20
	if (ret < 0)
		input_err(true, &data->client->dev, "%s write fail %d\n",__func__, ret);

	if(en){
		ret = __i2c_reg_write(data->client, STMPE1801_REG_GPIO_SET_yyy, 0x20);		//gpio5 0x20
		if (ret < 0)
			input_err(true, &data->client->dev, "%s write fail %d\n",__func__, ret);
	}
	else{
		ret = __i2c_reg_write(data->client, STMPE1801_REG_GPIO_CLR_yyy, 0x20);		//gpio5 0x20
		if (ret < 0)
			input_err(true, &data->client->dev, "%s write fail %d\n",__func__, ret);
	}

	ret =__i2c_reg_read(data->client, STMPE1801_REG_GPIO_MP_yyy);
	input_info(true, &data->client->dev, "%s - MP:%x\n",__func__, ret);
}
EXPORT_SYMBOL(stmpe1801_gpio_enable);

#if defined(SUPPORT_STM_INT)
int stmpe_find_irq(unsigned int irq)
{
	struct stmpe1801_dev *data = copy_device_data;
	int i;
	int stmpe1801_int_data_size = sizeof(stmpe1801_int_data)/sizeof(struct stmpe1801_int_data);

	for (i=0 ; i < stmpe1801_int_data_size ; i++){
		if (irq == stmpe1801_int_data[i].gpio_num) {
			return i;
		}
	}
	input_info(true, &data->client->dev, "%s : Can not find supported gpio[%d]\n",__func__, irq);
	return -1;
}

int stmpe_enable_irq(unsigned int irq)
{
	struct stmpe1801_dev *data = copy_device_data;
	int gpio_index = 0;
	int ret;

	gpio_index = stmpe_find_irq(irq);
	if (gpio_index < 0){
		return -1;
	}

	ret = __i2c_set_bits(data->client, STMPE1801_REG_INT_GPIO_MASK_LOW + stmpe1801_int_data[gpio_index].offset_addr,
									stmpe1801_int_data[gpio_index].offset_bit, 1);
	if (ret < 0)
		input_err(true, &data->client->dev, "%s fail[%d].\n", __func__, irq);

	input_info(true, &data->client->dev, "%s Done[%d].\n", __func__, irq);
	return 0;
}
int stmpe_disable_irq(unsigned int irq)
{
	struct stmpe1801_dev *data = copy_device_data;
	int gpio_index = 0;
	int ret;

	gpio_index = stmpe_find_irq(irq);
	if (gpio_index < 0){
		return -1;
	}

	ret = __i2c_set_bits(data->client, STMPE1801_REG_INT_GPIO_MASK_LOW + stmpe1801_int_data[gpio_index].offset_addr,
									stmpe1801_int_data[gpio_index].offset_bit, 0);
	if (ret < 0)
		input_info(true, &data->client->dev, "%s fail[%d].\n", __func__, irq);

	input_info(true, &data->client->dev, "%s Done[%d].\n", __func__, irq);
	return 0;
}
/**
 * stmpe_request_irq()
 *
 * Set Gpio pin to IRQ
 * irq                  : Gpio number on 3X4 KeyPad(not use keypad gpio).
 * stmpe_irq_func : Interrupt handler
 * irqflags            : Set IRQ type : Only support Falling-Edge on Novel.
 * devname          : Device name
 * dev_id             : Device data
 */
int stmpe_request_irq(unsigned int irq, void (*stmpe_irq_func)(void *),
							unsigned long irqflags, char *devname, void *dev_id)
{
	struct stmpe1801_dev *data = copy_device_data;
	int gpio_index = 0;

	gpio_index = stmpe_find_irq(irq);

	if (gpio_index < 0) {
		input_err(true, &data->client->dev, "%s Can not find supported gpio[%d]\n",__func__, irq);
		return -1;
	}

	if (stmpe1801_int_data[gpio_index].irq_enable){
		input_err(true, &data->client->dev, "%s Already use gpio pin(%d)\n",__func__, irq);
		return -1;
	}

	stmpe1801_int_data[gpio_index].irq_enable = 1;
	stmpe1801_int_data[gpio_index].irq_type = irqflags;
	stmpe1801_int_data[gpio_index].device_name = devname;
	stmpe1801_int_data[gpio_index].stmpe_irq_func = stmpe_irq_func;
	stmpe1801_int_data[gpio_index].dev_id = dev_id;

	input_info(true, &data->client->dev, "%s : gpio_index[%d] gpio[%d] offset_bit[%d] device_name[%s] irq_type[%x].\n",
			__func__, gpio_index, stmpe1801_int_data[gpio_index].gpio_num,
			stmpe1801_int_data[gpio_index].offset_bit, stmpe1801_int_data[gpio_index].device_name,
			stmpe1801_int_data[gpio_index].irq_type);

	return 0;
}
#endif

static int stmpe1801_dev_pinctrl_configure(struct stmpe1801_dev *data, bool active)
{
	struct pinctrl_state *set_state;
	int retval;

	if (!data->pinctrl)
		return -ENODEV;

	input_info(true, &data->client->dev, "%s: %s\n", __func__, active ? "ACTIVE" : "SUSPEND");

	set_state = pinctrl_lookup_state(data->pinctrl, active ? "active_state" : "suspend_state");
	if (IS_ERR(set_state)) {
		input_err(true, &data->client->dev, "%s: cannot get active state\n", __func__);
		return -EINVAL;
	}

	retval = pinctrl_select_state(data->pinctrl, set_state);
	if (retval) {
		input_err(true, &data->client->dev, "%s: cannot set pinctrl %s state\n",
				__func__, active ? "active" : "suspend");
		return -EINVAL;
	}

	return 0;
}

static int stmpe1801_dev_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct input_dev *input_dev;
	struct stmpe1801_dev *device_data;
	int ret = 0;

	input_err(true, &client->dev, "%s++\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev,
			"i2c_check_functionality fail\n");
		return -EIO;
	}

	device_data = kzalloc(sizeof(struct stmpe1801_dev), GFP_KERNEL);
	if (!device_data) {
		input_err(true, &client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	copy_device_data = device_data;

	input_dev = input_allocate_device();
	if (!input_dev) {
		input_err(true, &client->dev,
			"Failed to allocate memory for input device\n");
		ret = -ENOMEM;
		goto err_input_alloc;
	}

	device_data->client = client;
	device_data->input_dev = input_dev;

	atomic_set(&device_data->dev_enabled, 0);
	atomic_set(&device_data->dev_init, 0);

	mutex_init(&device_data->dev_lock);
	wake_lock_init(&device_data->stmpe_wake_lock, WAKE_LOCK_SUSPEND, "stmpe_key wake lock");


	device_data->client = client;

	if (client->dev.of_node) {

		device_data->dtdata = devm_kzalloc(&client->dev,
			sizeof(struct stmpe1801_devicetree_data), GFP_KERNEL);
		if (!device_data->dtdata) {
			input_err(true, &client->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_config;
		}
		ret = stmpe1801_parse_dt(&client->dev, device_data);
		if (ret) {
			input_err(true, &client->dev, "Failed to use device tree\n");
			ret = -EIO;
			goto err_config;
		}

	} else {
		input_err(true, &client->dev, "No use device tree\n");
		device_data->dtdata = client->dev.platform_data;
	}

	if (device_data->dtdata == NULL) {
		input_err(true, &client->dev, "failed to get platform data\n");
		goto err_config;
	}
	/* Get pinctrl if target uses pinctrl */
	device_data->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(device_data->pinctrl)) {
		if (PTR_ERR(device_data->pinctrl) == -EPROBE_DEFER)
			goto err_config;

		input_err(true, &client->dev, "%s: Target does not use pinctrl\n", __func__);
		device_data->pinctrl = NULL;
	}

	stmpe1801_dev_pinctrl_configure(device_data, true);
	stmpe1801_regs_init(device_data);


	device_data->keycode_entries = __stmpe1801_get_keycode_entries(device_data,
					((1 << device_data->dtdata->num_row) - 1),
					((1 << device_data->dtdata->num_column) - 1));
	input_info(true, &client->dev, "%s keycode entries (%d)\n",__func__, device_data->keycode_entries);
	input_info(true, &client->dev, "%s keymap size (%d)\n",__func__, device_data->keymap_data->keymap_size);

	device_data->keycode = kzalloc(device_data->keycode_entries * sizeof(unsigned short), GFP_KERNEL);
	if (device_data->keycode == NULL) {
		input_err(true, &client->dev, "kzalloc memory error\n");
		goto err_keycode;
	}
	input_info(true, &client->dev, "%s keycode addr (%p)\n", __func__, device_data->keycode);

	i2c_set_clientdata(client, device_data);

	ret = stmpe1801_dev_regulator_on(&client->dev, device_data);
	if (ret < 0) {
		dev_err(&client->dev, "stmpe1801_dev_regulator_on error... ignoring\n");
		ret = 0;
	}

	input_err(true, &client->dev, "%s: int:%d, sda:%d, scl:%d, rst:%d\n", __func__,
		gpio_get_value(device_data->dtdata->gpio_int),
		gpio_get_value(device_data->dtdata->gpio_sda),
		gpio_get_value(device_data->dtdata->gpio_scl),
		gpio_get_value(device_data->dtdata->gpio_rst));

	ret = stmpe1801_dev_power_on(device_data);
	if (ret < 0)
	{
		input_err(true, &client->dev, "stmpe1801_dev_power_on error\n");
		goto err_power;
	}
	ret = stmpe1801_dev_power_off(device_data);
	if (ret < 0)
	{
		input_err(true, &client->dev, "stmpe1801_dev_power_off error\n");
		goto err_power;
	}

	device_data->input_dev->dev.parent = &client->dev;
	device_data->input_dev->name = "sec_keypad_3x4-keypad";
	device_data->input_dev->id.bustype = BUS_I2C;
	device_data->input_dev->flush = NULL;
	device_data->input_dev->event = NULL;
	device_data->input_dev->open = stmpe1801_dev_kpc_input_open;
	device_data->input_dev->close = stmpe1801_dev_kpc_input_close;


	input_set_drvdata(device_data->input_dev, device_data);
	input_set_capability(device_data->input_dev, EV_MSC, MSC_SCAN);
	__set_bit(EV_KEY, device_data->input_dev->evbit);
	if (device_data->dtdata->repeat)
		__set_bit(EV_REP, device_data->input_dev->evbit);

	device_data->input_dev->keycode = device_data->keycode;
	device_data->input_dev->keycodesize = sizeof(unsigned short);
	device_data->input_dev->keycodemax = device_data->keycode_entries;

	matrix_keypad_build_keymap(device_data->keymap_data, NULL,
		device_data->dtdata->num_row, device_data->dtdata->num_column, device_data->keycode, device_data->input_dev);

// request irq
	if (device_data->dtdata->gpio_int >= 0) {
		device_data->dev_irq = gpio_to_irq(device_data->dtdata->gpio_int);
		input_info(true, &client->dev, "%s INT mode (%d)\n", __func__, device_data->dev_irq);

		ret = request_threaded_irq(device_data->dev_irq, NULL, stmpe1801_dev_isr,
#ifdef STMPE1801_TRIGGER_TYPE_FALLING
			IRQF_TRIGGER_FALLING|IRQF_ONESHOT,
#else
			IRQF_TRIGGER_LOW|IRQF_ONESHOT,
#endif
			client->name, device_data);

		if (ret < 0) {
			input_err(true, &client->dev, "stmpe1801_dev_power_off error\n");
			goto interrupt_err;
		}
		disable_irq_nosync(device_data->dev_irq);
	}
	else {
		input_info(true, &client->dev, "%s poll mode\n", __func__);

		INIT_DELAYED_WORK(&device_data->worker, stmpe1801_dev_worker);
		schedule_delayed_work(&device_data->worker, STMPE1801_TMR_INTERVAL);
	}


	ret = input_register_device(device_data->input_dev);
	if (ret) {
		input_err(true, &client->dev, "stmpe1801_dev_power_off error\n");
		goto input_err;
	}


//	device_data->sec_keypad = sec_device_create(device_data, "sec_keypad");
	device_data->sec_keypad = sec_device_create(13, device_data, "sec_keypad");
	if (IS_ERR(device_data->sec_keypad))
		input_err(true, &client->dev, "Failed to create sec_key device\n");
	ret = sysfs_create_group(&device_data->sec_keypad->kobj, &key_attr_group);
	if (ret) {
		input_err(true, &client->dev, "Failed to create the test sysfs: %d\n",
			ret);
	}

	// NOVEL gpio 5 set LOW for TDMB_FM_SEL
	stmpe1801_gpio_enable(0);

	device_init_wakeup(&client->dev, 1);

	return ret;

	//sec_device_destroy(13);
input_err:
interrupt_err:
err_power:
	kfree(device_data->keycode);
err_keycode:
err_config:
	input_free_device(device_data->input_dev);
	mutex_destroy(&device_data->dev_lock);
	wake_lock_destroy(&device_data->stmpe_wake_lock);

err_input_alloc:
	copy_device_data = NULL;
	kfree(device_data);

	input_err(true, &client->dev, "Error at stmpe1801_dev_probe\n");

	return ret;
}

static int stmpe1801_dev_remove(struct i2c_client *client)
{
	struct stmpe1801_dev *device_data = i2c_get_clientdata(client);

	device_init_wakeup(&client->dev, 0);
	wake_lock_destroy(&device_data->stmpe_wake_lock);

	input_unregister_device(device_data->input_dev);
	input_free_device(device_data->input_dev);

	regulator_disable(device_data->dtdata->vdd_vreg);

	if (device_data->dtdata->gpio_int >= 0)
		free_irq(device_data->dev_irq, device_data);

	sec_device_destroy(13);

	kfree(device_data->keycode);
	kfree(device_data);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int stmpe1801_dev_suspend(struct device *dev)
{
	struct stmpe1801_dev *device_data = dev_get_drvdata(dev);

#ifdef DBG_PRN
	input_dbg(true, &device_data->client->dev, "%s\n", __func__);
#endif

	if (device_may_wakeup(dev)) {
		if (device_data->dtdata->gpio_int >= 0)
			enable_irq_wake(device_data->dev_irq);
	} else {
		mutex_lock(&device_data->input_dev->mutex);

		if (device_data->input_dev->users)
			stmpe1801_dev_kpc_disable(device_data);

		mutex_unlock(&device_data->input_dev->mutex);
	}

	stmpe1801_dev_pinctrl_configure(device_data, false);

	return 0;
}

#ifdef USE_SUSPEND_LATE
static int stmpe1801_dev_suspend_late(struct device *dev)
{
	struct stmpe1801_dev *device_data = dev_get_drvdata(dev);

#ifdef DBG_PRN
	input_dbg(true, &device_data->client->dev, "%s\n", __func__);
#endif
	device_data->dtdata->sleep_state = true;

	return 0;
}

static int stmpe1801_dev_resume_noirq(struct device *dev)
{
	struct stmpe1801_dev *device_data = dev_get_drvdata(dev);

#ifdef DBG_PRN
	input_dbg(true, &device_data->client->dev, "%s\n", __func__);
#endif
	device_data->dtdata->sleep_state = false;

	return 0;
}
#endif

static int stmpe1801_dev_resume(struct device *dev)
{
	struct stmpe1801_dev *device_data = dev_get_drvdata(dev);

#ifdef DBG_PRN
	input_dbg(true, &device_data->client->dev, "%s\n", __func__);
#endif

	stmpe1801_dev_pinctrl_configure(device_data, true);

	device_data->dtdata->sleep_state = false;

	if (device_may_wakeup(dev)) {
		if (device_data->dtdata->gpio_int >= 0)
			disable_irq_wake(device_data->dev_irq);
	} else {
		mutex_lock(&device_data->input_dev->mutex);

		if (device_data->input_dev->users)
			stmpe1801_dev_kpc_enable(device_data);

		mutex_unlock(&device_data->input_dev->mutex);
	}

	return 0;
}

#ifdef USE_SUSPEND_LATE
static const struct dev_pm_ops stmpe1801_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(stmpe1801_dev_suspend, stmpe1801_dev_resume)
	.suspend_late = stmpe1801_dev_suspend_late,
	.resume_noirq = stmpe1801_dev_resume_noirq,
};
#else
static SIMPLE_DEV_PM_OPS(stmpe1801_dev_pm_ops, stmpe1801_dev_suspend, stmpe1801_dev_resume);
#endif
#endif

static const struct i2c_device_id stmpe1801_dev_id[] = {
	{ STMPE1801_DRV_NAME, 0 },
	{ }
};

#ifdef CONFIG_OF
static struct of_device_id stmpe1801_match_table[] = {
	{ .compatible = "stmpe,stmpe1801bjr",},
	{ },
};
#else
#define abov_match_table NULL
#endif

static struct i2c_driver stmpe1801_dev_driver = {
	.driver = {
		.name = STMPE1801_DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = stmpe1801_match_table,
		.pm = &stmpe1801_dev_pm_ops,
	},
	.probe = stmpe1801_dev_probe,
	.remove = stmpe1801_dev_remove,
	.id_table = stmpe1801_dev_id,
};

static int __init stmpe1801_dev_init(void)
{
	pr_err("%s++\n", __func__);

	return i2c_add_driver(&stmpe1801_dev_driver);
}
static void __exit stmpe1801_dev_exit(void)
{
	i2c_del_driver(&stmpe1801_dev_driver);
}
module_init(stmpe1801_dev_init);
module_exit(stmpe1801_dev_exit);

MODULE_DEVICE_TABLE(i2c, stmpe1801_dev_id);

MODULE_DESCRIPTION("STMPE1801 Key Driver");
MODULE_AUTHOR("Gyusung SHIM <gyusung.shim@st.com>");
MODULE_LICENSE("GPL");
