/* Himax Android Driver Sample Code for HMX83100 chipset
*
* Copyright (C) 2015 Himax Corporation.
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

#include "himax_platform.h"


#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
#define D(x...) printk(KERN_DEBUG "[HXTSP] " x)
#define I(x...) printk(KERN_INFO "[HXTSP] " x)
#define W(x...) printk(KERN_WARNING "[HXTSP][WARNING] " x)
#define E(x...) printk(KERN_ERR "[HXTSP][ERROR] " x)
#endif

int irq_enable_count = 0;
bool tsp_pwr_status = 0;

#ifdef QCT
int i2c_himax_read(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &command,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		}
	};

	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2)
			break;
		msleep(10);
	}
	if (retry == toRetry) {
		E("%s: i2c_read_block retry over %d\n",
			__func__, toRetry);
		return -EIO;
	}
	return 0;

}

int i2c_himax_write(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry/*, loop_i*/;
	uint8_t buf[length + 1];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	buf[0] = command;
	memcpy(buf+1, data, length);
	
	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		msleep(10);
	}

	if (retry == toRetry) {
		E("%s: i2c_write_block retry over %d\n",
			__func__, toRetry);
		return -EIO;
	}
	return 0;

}

int i2c_himax_read_command(struct i2c_client *client, uint8_t length, uint8_t *data, uint8_t *readlength, uint8_t toRetry)
{
	int retry;
	struct i2c_msg msg[] = {
		{
		.addr = client->addr,
		.flags = I2C_M_RD,
		.len = length,
		.buf = data,
		}
	};

	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		msleep(10);
	}
	if (retry == toRetry) {
		E("%s: i2c_read_block retry over %d\n",
		       __func__, toRetry);
		return -EIO;
	}
	return 0;
}

int i2c_himax_write_command(struct i2c_client *client, uint8_t command, uint8_t toRetry)
{
	return i2c_himax_write(client, command, NULL, 0, toRetry);
}

int i2c_himax_master_write(struct i2c_client *client, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry/*, loop_i*/;
	uint8_t buf[length];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length,
			.buf = buf,
		}
	};

	memcpy(buf, data, length);
	
	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		msleep(10);
	}

	if (retry == toRetry) {
		E("%s: i2c_write_block retry over %d\n",
		       __func__, toRetry);
		return -EIO;
	}
	return 0;
}

void himax_int_enable(int irqnum, int enable)
{
	if (enable == 1 && irq_enable_count == 0) {
		enable_irq(irqnum);
		irq_enable_count++;
	} else if (enable == 0 && irq_enable_count == 1) {
		disable_irq_nosync(irqnum);
		irq_enable_count--;
	}
	I("irq_enable_count = %d", irq_enable_count);
}

void himax_rst_gpio_set(int pinnum, uint8_t value)
{
	gpio_direction_output(pinnum, value);
}

uint8_t himax_int_gpio_read(int pinnum)
{
	return gpio_get_value(pinnum);
}

#ifdef SEC_PWRCTL_WITHLCD
int himax_power_control(struct himax_i2c_platform_data *pdata, bool en, bool init)
{
	int rc = 0;
	//static struct regulator *ldo6;
	//const char *reg_name;

	//reg_name = "8916_l6";

	I("%s %s\n", __func__, (en) ? "on" : "off");

	if(tsp_pwr_status == en){
		E("%s already en:%d\n", __func__, en);
		return 0;
	}

	if(init){
		if(pdata->gpio_reset > 0){
			rc = gpio_direction_output(pdata->gpio_reset, 0);
			if (rc) {
				E("%s: unable to set_direction for gpio_reset [%d]\n",
					__func__, pdata->gpio_reset);
			}
		}
	}

/*	if (!ldo6) {
		ldo6 = regulator_get(NULL, reg_name);

		if (IS_ERR(ldo6)) {
			E("%s: could not get %s, rc = %ld\n",__func__, reg_name, PTR_ERR(ldo6));
			return -EINVAL;
		}
		rc = regulator_set_voltage(ldo6, 1800000, 1800000);
		if (rc)
			E("%s: %s set_level failed (%d)\n", __func__, reg_name, rc);
	} */


	/*Power from MFD internal LDO */
#if 0
	if(pdata->tsp_vdd_name) {
		if(!pdata->tsp_power) {
			pdata->tsp_power = regulator_get(NULL, pdata->tsp_vdd_name);

			if (IS_ERR(pdata->tsp_power)) {
				E("%s: could not get tsp_power, rc = %ld\n",
					__func__,PTR_ERR(pdata->tsp_power));
				return -EINVAL;
			}
			rc = regulator_set_voltage(pdata->tsp_power, 2800000, 2800000);
			if (rc)
				E("%s: %s set_level failed (%d)\n", __func__, pdata->tsp_vdd_name, rc);
		}
	}
	if (en) {
/*		rc = regulator_enable(ldo6);
		if (rc) {
			E("%s: %s enable failed (%d)\n", __func__, reg_name, rc);
			return -EINVAL;
		} */
		if(pdata->tsp_vdd_name) {
			rc = regulator_enable(pdata->tsp_power);
			if (rc) {
				E("%s: %s enable failed (%d)\n", __func__, pdata->tsp_vdd_name, rc);
				return -EINVAL;
			}
		}
	} else { 
/*		rc = regulator_disable(ldo6);
		if (rc) {
			E("%s: %s disable failed (%d)\n", __func__, reg_name, rc);
			return -EINVAL;
		} */
		if(pdata->tsp_vdd_name) {
			rc = regulator_disable(pdata->tsp_power);
			if (rc) {
				E("%s: %s disable failed (%d)\n", __func__, pdata->tsp_vdd_name, rc);
				return -EINVAL;
			}
		}
	}
#else
	rc = gpio_direction_output(pdata->gpio_1v8_en, en);
	if (rc) {
		E("%s: unable to set_direction for gpio_1v8_en [%d]\n",
			__func__, pdata->gpio_1v8_en);
	}

	rc = gpio_direction_output(pdata->gpio_3v3_en, en);
	if (rc) {
		E("%s: unable to set_direction for gpio_3v3_en [%d]\n",
			__func__, pdata->gpio_3v3_en);
	}

#endif
	rc = gpio_direction_output(pdata->gpio_iovcc_en, en);
	if (rc) {
		E("%s: unable to set_direction for gpio_iovcc_en [%d]\n",
			__func__, pdata->gpio_iovcc_en);
	}

	if(init){
		if(pdata->gpio_reset > 0){
			rc = gpio_direction_output(pdata->gpio_reset, 1);
			if (rc) {
				E("%s: unable to set_direction for gpio_reset [%d]\n",
					__func__, pdata->gpio_reset);
				return rc;
			}
			usleep_range(20 * 1000, 20 * 1000);
			gpio_direction_output(pdata->gpio_reset, 0);
			usleep_range(1 * 1000, 1 * 1000);
			gpio_direction_output(pdata->gpio_reset, 1);
			usleep_range(20 * 1000, 20 * 1000);
		}
	}
	tsp_pwr_status = en;

	return rc;

}
#else
int himax_gpio_power_config(struct i2c_client *client,struct himax_i2c_platform_data *pdata)
{
	int error=0;
	
	if (pdata->gpio_reset >= 0) {
		error = gpio_request(pdata->gpio_reset, "himax-reset");
		if (error < 0)
			E("%s: request reset pin failed\n", __func__);
	}

	if (pdata->gpio_3v3_en >= 0) {
		error = gpio_request(pdata->gpio_3v3_en, "himax-3v3_en");
		if (error < 0)
			E("%s: request 3v3_en pin failed\n", __func__);
		gpio_direction_output(pdata->gpio_3v3_en, 1);
		I("3v3_en pin =%d\n", gpio_get_value(pdata->gpio_3v3_en));
	}
	
return error;
}

#endif

#endif

