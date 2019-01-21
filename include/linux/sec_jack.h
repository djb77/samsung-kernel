/*
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#ifndef __ASM_ARCH_SEC_HEADSET_H
#define __ASM_ARCH_SEC_HEADSET_H

#ifdef __KERNEL__

enum sec_jack_type {
	SEC_JACK_NO_DEVICE				= 0x0,
	SEC_HEADSET_4POLE				= 0x01 << 0,
	SEC_HEADSET_3POLE				= 0x01 << 1,
	SEC_EXTERNAL_ANTENNA			= 0x01 << 2,
};

struct sec_jack_zone {
	unsigned int adc_high;
	unsigned int delay_us;
	unsigned int check_count;
	unsigned int jack_type;
};

struct sec_jack_buttons_zone {
	unsigned int code;
	unsigned int adc_low;
	unsigned int adc_high;
};

struct sec_jack_control_data {
	int snd_card_registered;
	void (*set_micbias)(bool);
	int (*get_adc)(void);
	void (*hp_imp_detect)(void);
	void (*hp_imp_unplug)(void);
	int jack_type;
};

struct sec_jack_platform_data {
	int det_gpio;
	int key_gpio;
	int ear_micbias_en_gpio;
	bool det_active_high;
	bool send_end_active_high;
	int key_debounce_time_ms;
	int det_debounce_time_ms;
	struct sec_jack_zone jack_zones[4];
	struct sec_jack_buttons_zone jack_buttons_zones[4];
	struct sec_jack_control_data *jack_controls;
	struct pinctrl *jack_pinctrl;
	struct pinctrl_state *jack_pins_active;
};

extern struct sec_jack_control_data jack_controls;

typedef void (*sec_jack_button_notify_cb)(int code, int event);
int sec_jack_register_button_notify_cb(sec_jack_button_notify_cb func);
int get_jack_state(void);
#endif

#endif
