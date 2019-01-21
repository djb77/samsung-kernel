#ifndef _MUIC_APIS_
#define _MUIC_APIS_

extern int attach_ta(muic_data_t *pmuic);
extern int detach_ta(muic_data_t *pmuic);
extern int do_BCD_rescan(muic_data_t *pmuic);
extern int enable_periodic_adc_scan(muic_data_t *pmuic);
extern int disable_periodic_adc_scan(muic_data_t *pmuic);
extern int com_to_open_with_vbus(muic_data_t *pmuic);
extern int com_to_usb(muic_data_t *pmuic);
extern int com_to_uart(muic_data_t *pmuic);
extern int com_to_audio(muic_data_t *pmuic);
extern int com_to_uart_ap(muic_data_t *pmuic);
extern int switch_to_ap_usb(muic_data_t *pmuic);
extern int switch_to_ap_uart(muic_data_t *pmuic);
extern int switch_to_cp_uart(muic_data_t *pmuic);
extern int attach_usb_util(muic_data_t *pmuic,
			muic_attached_dev_t new_dev);
extern int attach_usb(muic_data_t *pmuic,
			muic_attached_dev_t new_dev);
extern int detach_usb(muic_data_t *pmuic);
extern int attach_otg_usb(muic_data_t *pmuic,
			muic_attached_dev_t new_dev);
extern int detach_otg_usb(muic_data_t *pmuic);
extern int attach_ps_cable(muic_data_t *pmuic,
			muic_attached_dev_t new_dev);
extern int detach_ps_cable(muic_data_t *pmuic);
extern int attach_ps_cable(muic_data_t *pmuic,
			muic_attached_dev_t new_dev);
extern int attach_gamepad(muic_data_t *pmuic,
			muic_attached_dev_t new_dev);
extern int detach_gamepad(muic_data_t *pmuic);
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
extern void set_earjack_mode(muic_data_t *pmuic, int mode);
extern void set_earjack_state(muic_data_t *pmuic, int intr1, int intr2, int btn1, int btn2);
extern int get_attached_earjack_type(muic_data_t *pmuic);
extern int attach_earjack(muic_data_t *pmuic,
			muic_attached_dev_t new_dev);
extern int detach_earjack(muic_data_t *pmuic);

extern int attach_long_earjackkey(muic_data_t *pmuic,
			muic_attached_dev_t new_dev);
extern int detach_long_earjackkey(muic_data_t *pmuic,
			muic_attached_dev_t new_dev);
#endif
extern int attach_deskdock(muic_data_t *pmuic,
			muic_attached_dev_t new_dev);
extern int detach_deskdock(muic_data_t *pmuic);
extern int attach_audiodock(muic_data_t *pmuic,
			muic_attached_dev_t new_dev, u8 vbus);
extern int detach_audiodock(muic_data_t *pmuic);
extern int attach_jig_uart_boot_off(muic_data_t *pmuic, muic_attached_dev_t new_dev,
				u8 vbvolt);
extern int detach_jig_uart_boot_off(muic_data_t *pmuic);
extern int attach_jig_uart_boot_on(muic_data_t *pmuic, muic_attached_dev_t new_dev);
extern int detach_jig_uart_boot_on(muic_data_t *pmuic);
extern int attach_jig_usb_boot_off(muic_data_t *pmuic,
				u8 vbvolt);
extern int attach_jig_usb_boot_on(muic_data_t *pmuic,
				u8 vbvolt);
extern int attach_mhl(muic_data_t *pmuic);
extern int detach_mhl(muic_data_t *pmuic);
extern int get_adc(muic_data_t *pmuic);
extern int get_vps_data(muic_data_t *pmuic, void *pdata);
extern void set_switch_mode(muic_data_t *pmuic, int mode);
extern int get_switch_mode(muic_data_t *pmuic);
extern void set_adc_scan_mode(muic_data_t *pmuic,const u8 val);
extern int get_adc_scan_mode(muic_data_t *pmuic);
extern int enable_chgdet(muic_data_t *pmuic, int enable);
extern int run_chgdet(muic_data_t *pmuic, bool started);
#endif
