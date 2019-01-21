/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
*
* File Name		: stmpe1801_key.h
* Authors		: AMS Korea TC
*				: Gyusung SHIM
* Version		: V 2.4
* Date			: 06/14/2012
* Description	: STMPE1801 driver for keypad function
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
*
********************************************************************************
* REVISON HISTORY
*
* VERSION | DATE       | AUTHORS	     | DESCRIPTION
* 1.0	  | 04/04/2012 | Gyusung SHIM    | First release for Kernel 3.x
* 1.1	  | 04/06/2012 | Gyusung SHIM    | ISR enable
* 1.2	  | 04/11/2012 | Gyusung SHIM    | CONFIG_PM support
* 1.3	  | 04/12/2012 | Gyusung SHIM    | Refine enable/disable & bugfix
* 1.4	  | 04/12/2012 | Gyusung SHIM    | Adding sysfs
* 1.5	  | 04/13/2012 | Gyusung SHIM    | fix kzalloc : size of pdata
* 1.6	  | 04/18/2012 | Gyusung SHIM    | fix 'up' in stmpe1801_key_int_proc is always 0
* 1.7	  | 04/18/2012 | Gyusung SHIM    | fix __stmpe1801_get_keycode_entries reports wrong value
* 1.8	  | 04/18/2012 | Gyusung SHIM    | add #if switch on disable/enable irq in isr
* 1.9	  | 05/09/2012 | Gyusung SHIM    | Spliting pinmask to row & col, optimizing isr.
* 2.0	  | 05/16/2012 | Gyusung SHIM    | Add ghost key list.
* 2.1	  | 06/01/2012 | Gyusung SHIM    | Fix enable/disable return & remove lock in probe
* 2.2	  | 06/08/2012 | Gyusung SHIM    | Revise PM functions
* 2.3	  | 06/13/2012 | Gyusung SHIM    | Fix STMPE1801_REG_SYS_CTRL
* 2.4	  | 06/14/2012 | Gyusung SHIM    | Add STMPE1801_TRIGGER_TYPE_LEVEL config
*
*******************************************************************************/
#ifndef __LINUX_INPUT_STMPE1801_KEY_H
#define __LINUX_INPUT_STMPE1801_KEY_H

#include <linux/i2c.h>

#define STMPE1801_DRV_DESC				"stmpe1801 i2c I/O expander"
#define STMPE1801_DRV_NAME				"stmpe1801_key"


#define STMPE1801_BLOCK_GPIO			0x01
#define STMPE1801_BLOCK_KEY			0x02

#define STMPE1801_DEBOUNCE_30US			0
#define STMPE1801_DEBOUNCE_90US			(1 << 1)
#define STMPE1801_DEBOUNCE_150US		(2 << 1)
#define STMPE1801_DEBOUNCE_210US		(3 << 1)

#define STMPE1801_SCAN_FREQ_MASK		0x03
#define STMPE1801_SCAN_FREQ_60HZ		0x00
#define STMPE1801_SCAN_FREQ_30HZ		0x01
#define STMPE1801_SCAN_FREQ_15HZ		0x02

#define STMPE1801_SCAN_FREQ_275HZ		0x03

#define STMPE1801_SCAN_CNT_MASK			0xF0
#define STMPE1801_SCAN_CNT_SHIFT		4

#define STMPE1801_INT_CTRL_ACTIVE_LOW		0x00
#define STMPE1801_INT_CTRL_ACTIVE_HIGH		0x04
#define STMPE1801_INT_CTRL_TRIG_LEVEL		0x00
#define STMPE1801_INT_CTRL_TRIG_EDGE		0x02
#define STMPE1801_INT_CTRL_GLOBAL_ON		0x01
#define STMPE1801_INT_CTRL_GLOBAL_OFF		0x00

#define STMPE1801_INT_MASK_GPIO			0x08
#define STMPE1801_INT_MASK_KEY			0x02
#define STMPE1801_INT_MASK_KEY_COMBI		0x10
#define STMPE1801_INT_MASK_KEY_FIFO		0x04

#define STMPE1801_INT_STA_WAKEUP		0x01
#define STMPE1801_INT_STA_KEY			0x02
#define STMPE1801_INT_STA_KEY_FIFO		0x04
#define STMPE1801_INT_STA_GPIO			0x08
#define STMPE1801_INT_STA_KEY_COMBI		0x10

// row_1, col_1 & row_2, col_2 : 2 keys which will trigger the ghost key.
// row_g, col_g : the key which will be discarded.
#define STMPE1801_GHOST_KEY_VAL(row_1, col_1, row_2, col_2, row_g, col_g)	\
			(((row_1)&0x7)|(((col_1)&0xf)<< 3)			\
			|(((row_2)&0x7)<< 8)|(((col_2)&0xf)<<11)		\
			|(((row_g)&0x7)<<16)|(((col_g)&0xf)<<19))


struct stmpe1801_keypad_data {
//	struct matrix_keymap_data		*keymap_data;
//	struct matrix_keymap_data		*ghost_data;
//	unsigned int				scan_freq;
//	unsigned int				scan_cnt;
//	bool					autorepeat;
};


/**
 * struct stmpe_platform_data - STMPE platform data
 * @id: device id to distinguish between multiple STMPEs on the same board
 * @blocks: bitmask of blocks to enable (use STMPE_BLOCK_*)
 * @irq_trigger: IRQ trigger to use for the interrupt to the host
 * @irq_invert_polarity: IRQ line is connected with reversed polarity
 * @irq_base: base IRQ number.  %STMPE_NR_IRQS irqs will be used, or
 *	      %STMPE_NR_INTERNAL_IRQS if the GPIO driver is not used.
 * @gpio: GPIO-specific platform data
 * @keypad: keypad-specific platform data
 */
struct stmpe1801_platform_data {
	unsigned int blocks;
	unsigned int debounce;

	// pinmask order
	// Bit7	Bit6	...	Bit1	Bit0
	// Row7	Row6	...	Row1	Row0
	// u32	pinmask_keypad_row;
	// pinmask order
	// Bit9	Bit8	...	Bit1	Bit0
	// Col9	Col8	...	Col1	Col0
	// u32	pinmask_keypad_col;

	int int_gpio_no;
	int wake_up;

	struct stmpe1801_keypad_data	*keypad;

	int (*power_on)(struct i2c_client *client);
	int (*power_off)(struct i2c_client *client);
};

#define SUPPORT_STM_INT
#ifdef SUPPORT_STM_INT

#define STMPE_TRIGGER_TYPE_EDGE			(1 << 1)
#define STMPE_TRIGGER_TYPE_LEVEL		(0 << 1)
#define STMPE_TRIGGER_POLARITY_HIGH_RISING	(1 << 2)
#define STMPE_TRIGGER_POLARITY_LOW_FALLING	(0 << 2)

#define STMPE_TRIGGER_RISING	(STMPE_TRIGGER_TYPE_EDGE | STMPE_TRIGGER_POLARITY_HIGH_RISING)
#define STMPE_TRIGGER_FALLING	(STMPE_TRIGGER_TYPE_EDGE | STMPE_TRIGGER_POLARITY_LOW_FALLING)
#define STMPE_TRIGGER_HIGH	(STMPE_TRIGGER_TYPE_LEVEL | STMPE_TRIGGER_POLARITY_HIGH_RISING)
#define STMPE_TRIGGER_LOW	(STMPE_TRIGGER_TYPE_LEVEL | STMPE_TRIGGER_POLARITY_LOW_FALLING)

struct stmpe1801_int_data {
	int gpio_num;
	int irq_enable;
	int irq_type;
	int offset_addr;
	int offset_bit;
	void (*stmpe_irq_func)(void *);
	char *device_name;
	void *dev_id;
};
int stmpe_enable_irq(unsigned int irq);
int stmpe_disable_irq(unsigned int irq);

int stmpe_request_irq(unsigned int irq, void (*stmpe_irq_func)(void *),
			unsigned long irqflags, char *devname, void *dev_id);
#endif

void stmpe1801_gpio_enable(bool en);

#endif
