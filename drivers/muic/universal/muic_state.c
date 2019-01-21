/*
 * muic_state.c
 *
 * Copyright (C) 2014 Samsung Electronics
 * Thomas Ryu <smilesr.ryu@samsung.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/host_notify.h>

#include <linux/muic/muic.h>
#include <linux/usb_notify.h>

#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic_notifier.h>
#endif /* CONFIG_MUIC_NOTIFIER */

#if defined(CONFIG_VBUS_NOTIFIER)
#include <linux/vbus_notifier.h>
#endif /* CONFIG_VBUS_NOTIFIER */

#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif /* CONFIG_OF */

#include "muic-internal.h"
#include "muic_apis.h"
#include "muic_i2c.h"
#include "muic_vps.h"
#include "muic_regmap.h"
#include "muic_debug.h"

#if defined(CONFIG_MUIC_SUPPORT_CCIC)
#include "muic_ccic.h"
#endif

#if defined(CONFIG_MUIC_UNIVERSAL_SM5705)
#include <linux/muic/muic_afc.h>
#endif
//#include "linux/muic/sm5705-muic.h"

#if defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
extern void muic_afc_delay_check_state(int state);
#endif

#if defined(CONFIG_MUIC_UNIVERSAL_SM5708)
extern void sm5708_vdp_src_en_state(muic_data_t *muic, int state);
#endif

extern void muic_send_dock_intent(int type);

extern int muic_wakeup_noti;

void muic_set_wakeup_noti(int flag)
{
	pr_info("%s: %d\n", __func__, flag);
	muic_wakeup_noti = flag;
}

static void muic_handle_attach(muic_data_t *pmuic,
			muic_attached_dev_t new_dev, int adc, u8 vbvolt)
{
	int ret = 0;
	bool noti_f = true;
#if defined(CONFIG_MUIC_UNIVERSAL_SM5703) && !defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
	struct vendor_ops *pvendor = pmuic->regmapdesc->vendorops;

	/* W/A of sm5703 lanhub issue */
	if ((new_dev == ATTACHED_DEV_OTG_MUIC) && (vbvolt > 0)) {
		if (pvendor && pvendor->reset_vbus_path) {
			pr_info("%s:%s reset vbus path\n", MUIC_DEV_NAME, __func__);
			pvendor->reset_vbus_path(pmuic->regmapdesc);
		} else
			pr_info("%s: No Vendor API ready.\n", __func__);
	}
#endif

	pr_info("%s:%s attached_dev:%d new_dev:%d adc:0x%02x, vbvolt:%02x\n",
		MUIC_DEV_NAME, __func__, pmuic->attached_dev, new_dev, adc, vbvolt);

	if((new_dev == pmuic->attached_dev) &&
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
		(new_dev != ATTACHED_DEV_EARJACK_MUIC) &&
#endif
		(new_dev != ATTACHED_DEV_JIG_UART_OFF_MUIC)) {
		pr_info("%s:%s Duplicated device %d just ignore\n",
				MUIC_DEV_NAME, __func__,pmuic->attached_dev);
		return;
	}
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
	/* when earjack key is too short, do nothing */
	if (new_dev == ATTACHED_ERROR_EARJACK_MUIC) {
		pr_info("%s: earjack key press is to short.\n", __func__);
		return;	
	}
#endif
	switch (pmuic->attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
		if (new_dev != pmuic->attached_dev) {
			pr_warn("%s:%s new(%d)!=attached(%d), assume detach\n",
					MUIC_DEV_NAME, __func__, new_dev,
					pmuic->attached_dev);
			ret = detach_usb(pmuic);
		}
		break;
	case ATTACHED_DEV_HMT_MUIC:
	case ATTACHED_DEV_OTG_MUIC:
	/* OTG -> LANHUB, meaning TA is attached to LANHUB(OTG)
	   OTG -> GAMEPAD, meaning Earphone is detached to GAMEPAD(OTG) */
#if defined(CONFIG_MUIC_SUPPORT_LANHUB)
		if (new_dev == ATTACHED_DEV_USB_LANHUB_MUIC) {
			pr_info("%s:%s OTG+TA=>LANHUB. Do not detach OTG.\n",
					__func__, MUIC_DEV_NAME);
			noti_f = false;
			break;
		}
#endif
		if (new_dev == ATTACHED_DEV_GAMEPAD_MUIC) {
			pr_info("%s:%s OTG+No Earphone=>GAMEPAD. Do not detach OTG.\n",
					__func__, MUIC_DEV_NAME);
			noti_f = false;
			break;
		}

		if (new_dev == pmuic->attached_dev) {
			noti_f = false;
			break;

		/* Treat unexpected situations on disconnection */
		} else if (pmuic->is_gamepad && (adc == ADC_OPEN)) {
			pmuic->is_gamepad = false;
			muic_send_dock_intent(MUIC_DOCK_DETACHED);
		}
#if defined(CONFIG_MUIC_SUPPORT_LANHUB)
	case ATTACHED_DEV_USB_LANHUB_MUIC:
		if (new_dev != pmuic->attached_dev) {
			pr_warn("%s:%s new(%d)!=attached(%d), assume detach\n",
					MUIC_DEV_NAME, __func__, new_dev,
					pmuic->attached_dev);
			ret = detach_otg_usb(pmuic);
		}
		break;
#endif
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		if (new_dev != pmuic->attached_dev) {
			pr_warn("%s:%s new(%d)!=attached(%d), assume detach\n",
					MUIC_DEV_NAME, __func__, new_dev,
					pmuic->attached_dev);
			ret = detach_audiodock(pmuic);
		}
		break;

	case ATTACHED_DEV_TA_MUIC:
		pmuic->attached_dev = ATTACHED_DEV_NONE_MUIC;
		break;
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		if (new_dev != ATTACHED_DEV_JIG_UART_OFF_MUIC) {
			pr_warn("%s:%s new(%d)!=attached(%d), assume detach\n",
					MUIC_DEV_NAME, __func__, new_dev,
					pmuic->attached_dev);
			ret = detach_jig_uart_boot_off(pmuic);
		}
		break;

	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		if (new_dev != pmuic->attached_dev) {
			pr_warn("%s:%s new(%d)!=attached(%d), assume detach\n",
				MUIC_DEV_NAME, __func__, new_dev,
				pmuic->attached_dev);

			if (pmuic->is_factory_start)
				ret = detach_deskdock(pmuic);
			else
				ret = detach_jig_uart_boot_on(pmuic);

			muic_set_wakeup_noti(pmuic->is_factory_start ? 1: 0);
		}
		break;
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		if (new_dev == ATTACHED_DEV_DESKDOCK_MUIC || 
				new_dev == ATTACHED_DEV_DESKDOCK_VB_MUIC) {
			pr_warn("%s:%s new(%d)!=attached(%d), assume same device\n",
					MUIC_DEV_NAME, __func__, new_dev,
					pmuic->attached_dev);
			noti_f = false;
		} else if (new_dev != pmuic->attached_dev) {
			pr_warn("%s:%s new(%d)!=attached(%d), assume detach\n",
					MUIC_DEV_NAME, __func__, new_dev,
					pmuic->attached_dev);
			ret = detach_deskdock(pmuic);
		}
		break;
	case ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC:
		if (new_dev != pmuic->attached_dev) {
			pr_warn("%s:%s new(%d)!=attached(%d), assume detach\n",
					MUIC_DEV_NAME, __func__, new_dev,
					pmuic->attached_dev);
			ret = detach_otg_usb(pmuic);
		}
		break;
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
		if (new_dev != pmuic->attached_dev) {
			pr_warn("%s:%s new(%d)!=attached(%d), assume detach\n",
					MUIC_DEV_NAME, __func__, new_dev,
					pmuic->attached_dev);
			ret = detach_ps_cable(pmuic);
		}
		break;
	case ATTACHED_DEV_GAMEPAD_MUIC:
		if (new_dev == ATTACHED_DEV_OTG_MUIC) {
			pr_info("%s:%s GAMEPAD+Earphone=>OTG. Do not detach gamepad.\n",
					__func__, MUIC_DEV_NAME);
			noti_f = false;

			/* We have to inform the framework of this change
			  * if USB does not send a noti of gamepad to muic.
			  */
			muic_send_dock_intent(MUIC_DOCK_GAMEPAD_WITH_EARJACK);
		}
		break;		
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
		break;

	default:
		noti_f = false;
	}

	if (noti_f) {
		MUIC_INFO("muic_notifier_detach_attached_dev: %d\n", pmuic->attached_dev);
		muic_notifier_detach_attached_dev(pmuic->attached_dev);
	}

	noti_f = true;
	switch (new_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
		ret = attach_usb(pmuic, new_dev);
		break;
	case ATTACHED_DEV_HMT_MUIC:
	case ATTACHED_DEV_OTG_MUIC:
		ret = attach_otg_usb(pmuic, new_dev);
		break;
#if defined(CONFIG_MUIC_SUPPORT_LANHUB)
	case ATTACHED_DEV_USB_LANHUB_MUIC:
		ret = attach_otg_usb(pmuic, new_dev);
		break;
#endif
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		ret = attach_audiodock(pmuic, new_dev, vbvolt);
		break;
	case ATTACHED_DEV_TA_MUIC:
		attach_ta(pmuic);
		pmuic->attached_dev = new_dev;
#if defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
#if defined(CONFIG_MUIC_SUPPORT_CCIC)
		if (pmuic->afc_water_disable)
			pr_info("%s:%s AFC Disable(%d) by WATER!\n", MUIC_DEV_NAME,
				    __func__, pmuic->afc_water_disable);
#endif
		muic_afc_delay_check_state(1);
#endif
        
#ifdef CONFIG_MUIC_SM5705_AFC_18W_TA_SUPPORT
       pmuic->is_18w_ta = 0;
#endif

#if defined(CONFIG_MUIC_UNIVERSAL_SM5708)
		sm5708_vdp_src_en_state(pmuic, 1);
#endif
		break;
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		new_dev = attach_jig_uart_boot_off(pmuic, new_dev, vbvolt);
		break;
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		/* Keep AP UART path and
		 *  call attach_deskdock to wake up the device in the Facory Build Binary.
		 */
		 if (pmuic->is_factory_start)
			ret = attach_deskdock(pmuic, new_dev);
		else
			ret = attach_jig_uart_boot_on(pmuic, new_dev);

		muic_set_wakeup_noti(pmuic->is_factory_start ? 1: 0);
		break;
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		ret = attach_jig_usb_boot_off(pmuic, vbvolt);
		break;
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		ret = attach_jig_usb_boot_on(pmuic, vbvolt);
		break;
	case ATTACHED_DEV_MHL_MUIC:
		ret = attach_mhl(pmuic);
		break;
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		ret = attach_deskdock(pmuic, new_dev);
		break;
	case ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC:
		ret = attach_otg_usb(pmuic, new_dev);
		if (!vbvolt)
			noti_f = false;
		break;
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
		ret = attach_ps_cable(pmuic, new_dev);
		break;
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
		if (adc == 0x1E && vbvolt)
			detach_earjack(pmuic);
#endif
		com_to_open_with_vbus(pmuic);
		pmuic->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_VZW_INCOMPATIBLE_MUIC:
		com_to_open_with_vbus(pmuic);
		pmuic->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_GAMEPAD_MUIC:
		ret = attach_gamepad(pmuic, new_dev);
		break;
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
	case ATTACHED_DEV_SEND_MUIC:
	case ATTACHED_DEV_VOLDN_MUIC:
	case ATTACHED_DEV_VOLUP_MUIC:
		if (pmuic->attached_dev == ATTACHED_DEV_EARJACK_MUIC)
			pmuic->is_earkeypressed = true;
		
		ret = attach_earjack(pmuic, new_dev);
		break;
#if defined(CONFIG_MUIC_UNIVERSAL_SM5703)
	case ATTACHED_DEV_LP_SEND_MUIC:
	case ATTACHED_DEV_LP_VOLUP_MUIC:
	case ATTACHED_DEV_LP_VOLDN_MUIC:
		pmuic->is_earkeypressed = true;
		ret = attach_long_earjackkey(pmuic, new_dev);
		break;
	case ATTACHED_DEV_LR_SEND_MUIC:
	case ATTACHED_DEV_LR_VOLUP_MUIC:
	case ATTACHED_DEV_LR_VOLDN_MUIC:
		pmuic->is_earkeypressed = false;
		ret = detach_long_earjackkey(pmuic, new_dev);
		break;
	case ATTACHED_STUCK_EARJACK_MUIC:
#endif
	case ATTACHED_DEV_EARJACK_MUIC:
		ret = attach_earjack(pmuic, new_dev);
		break;
#endif
	case ATTACHED_DEV_UART_MUIC:
		pr_info("%s:%s ATTACHED_DEV_UART_MUIC\n", MUIC_DEV_NAME, __func__);
		ret = com_to_uart_ap(pmuic);
		break;
	default:
		pr_warn("%s:%s unsupported dev=%d, adc=0x%x, vbus=%c\n",
				MUIC_DEV_NAME, __func__, new_dev, adc,
				(vbvolt ? 'O' : 'X'));
		break;
	}

	if (noti_f) {
		MUIC_INFO("muic_notifier_attach_attached_dev: %d\n", pmuic->attached_dev);
		muic_notifier_attach_attached_dev(new_dev);
	} else {
		MUIC_INFO("muic attach Noti. for (%d) discarded\n", new_dev);
		pr_info("%s:%s attach Noti. for (%d) discarded.\n",
				MUIC_DEV_NAME, __func__, new_dev);
	}

	if (ret < 0)
		pr_warn("%s:%s something wrong with attaching %d (ERR=%d)\n",
				MUIC_DEV_NAME, __func__, new_dev, ret);
}

static void muic_handle_detach(muic_data_t *pmuic)
{
	int ret = 0;
	bool noti_f = true;

	pr_info("%s:%s attached_dev 0x(%x).\n", MUIC_DEV_NAME, __func__, pmuic->attached_dev);

	ret = com_to_open_with_vbus(pmuic);
#if defined(CONFIG_MUIC_UNIVERSAL_SM5705)  
    #ifndef CONFIG_MUIC_UNIVERSAL_MULTI_SUPPORT
	// ENAFC set '0'
	pmuic->regmapdesc->afcops->afc_ctrl_reg(pmuic->regmapdesc, AFCCTRL_ENAFC, 0);
    #else
        if(pmuic->is_afc_support == true)
            pmuic->regmapdesc->afcops->afc_ctrl_reg(pmuic->regmapdesc, AFCCTRL_ENAFC, 0);
    #endif
#endif
	//muic_enable_accdet(pmuic);
#ifdef CONFIG_MUIC_SM5705_AFC_18W_TA_SUPPORT
     pmuic->is_18w_ta = 0;
#endif

	switch (pmuic->attached_dev) {
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
		ret = detach_usb(pmuic);
		break;
	case ATTACHED_DEV_OTG_MUIC:
		if (pmuic->is_gamepad) {
			pmuic->is_gamepad = false;
			muic_send_dock_intent(MUIC_DOCK_DETACHED);
		}
	case ATTACHED_DEV_HMT_MUIC:
		ret = detach_otg_usb(pmuic);
		break;
#if defined(CONFIG_MUIC_SUPPORT_LANHUB)
	case ATTACHED_DEV_USB_LANHUB_MUIC:
		ret = detach_otg_usb(pmuic);
		break;
#endif
	case ATTACHED_DEV_TA_MUIC:
		pmuic->attached_dev = ATTACHED_DEV_NONE_MUIC;
		detach_ta(pmuic);
#if defined(CONFIG_MUIC_UNIVERSAL_SM5708)
		sm5708_vdp_src_en_state(pmuic, 0);
#endif
		break;
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		ret = detach_jig_uart_boot_off(pmuic);
		break;
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		if (pmuic->is_factory_start)
			ret = detach_deskdock(pmuic);
		else
			ret = detach_jig_uart_boot_on(pmuic);

		muic_set_wakeup_noti(pmuic->is_factory_start ? 1: 0);
		break;
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		ret = detach_deskdock(pmuic);
		break;
	case ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC:
		ret = detach_otg_usb(pmuic);
		break;
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		ret = detach_audiodock(pmuic);
		break;
	case ATTACHED_DEV_MHL_MUIC:
		ret = detach_mhl(pmuic);
		break;
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
		ret = detach_ps_cable(pmuic);
		break;
	case ATTACHED_DEV_NONE_MUIC:
		pmuic->is_afc_device = 0;
		pr_info("%s:%s duplicated(NONE)\n", MUIC_DEV_NAME, __func__);
		break;
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
		pr_info("%s:%s UNKNOWN\n", MUIC_DEV_NAME, __func__);
		pmuic->attached_dev = ATTACHED_DEV_NONE_MUIC;
		break;
	case ATTACHED_DEV_GAMEPAD_MUIC:
		ret = detach_gamepad(pmuic);
		break;
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
	case ATTACHED_DEV_EARJACK_MUIC:
		ret = detach_earjack(pmuic);
		break;
#endif
	case ATTACHED_DEV_UART_MUIC:
		pr_info("%s:%s detach ATTACHED_DEV_UART_MUIC\n", MUIC_DEV_NAME, __func__);
		//ret = com_to_uart_ap(pmuic);		
	default:
		pmuic->is_afc_device = 0;
		pr_info("%s:%s invalid attached_dev type(%d)\n", MUIC_DEV_NAME,
			__func__, pmuic->attached_dev);
		pmuic->attached_dev = ATTACHED_DEV_NONE_MUIC;
		break;
	}

	pmuic->muic_reset_count = 0;
		
	if (noti_f) {
		MUIC_INFO("muic_notifier_detach_attached_dev: %d\n", pmuic->attached_dev);
		muic_notifier_detach_attached_dev(pmuic->attached_dev);
	} else {
		MUIC_INFO("muic detach Noti. for (%d) discarded\n", pmuic->attached_dev);
		pr_info("%s:%s detach Noti. for (%d) discarded.\n",
				MUIC_DEV_NAME, __func__, pmuic->attached_dev);
	}
	if (ret < 0)
		pr_warn("%s:%s something wrong with detaching %d (ERR=%d)\n",
				MUIC_DEV_NAME, __func__, pmuic->attached_dev, ret);
}

void muic_detect_dev(muic_data_t *pmuic)
{
	muic_attached_dev_t new_dev = ATTACHED_DEV_UNKNOWN_MUIC;
	int intr = MUIC_INTR_DETACH;
#if defined(CONFIG_MUIC_SUPPORT_CCIC)
	struct vendor_ops *pvendor = pmuic->regmapdesc->vendorops;
	bool dcd;
#endif
	get_vps_data(pmuic, &pmuic->vps);

	MUIC_INFO_K("%s:%s dev[1:0x%x, 2:0x%x, 3:0x%x], adc:0x%x, vbvolt:0x%x\n",
		MUIC_DEV_NAME, __func__, pmuic->vps.s.val1, pmuic->vps.s.val2,
		pmuic->vps.s.val3, pmuic->vps.s.adc, pmuic->vps.s.vbvolt);
	pr_info("%s:%s intr[1:0x%x, 2:0x%x, 3:0x%x]\n", pmuic->chip_name, __func__,
			pmuic->intr.intr1, pmuic->intr.intr2, pmuic->intr.intr3);

	if (vps_resolve_dev(pmuic, &new_dev, &intr) < 0) {
		MUIC_INFO_K("%s:%s: discarded.\n",MUIC_DEV_NAME,__func__);
		return;
	}

	pr_info("%s:%s after resolve: new_dev:0x%x, intr:0x%x\n",
		MUIC_DEV_NAME, __func__, new_dev, intr);

#if defined(CONFIG_MUIC_SUPPORT_CCIC)
	if (pvendor->get_dcdtmr_irq) {
		dcd = pvendor->get_dcdtmr_irq(pmuic->regmapdesc);
		pr_info("%s:%s: get dcd timer state: %s\n",
				MUIC_DEV_NAME, __func__, dcd ? "true": "false");
		pmuic->is_dcdtmr_intr = dcd;
	}

	if (new_dev == ATTACHED_DEV_USB_MUIC && pmuic->is_dcdtmr_intr) {
		new_dev = ATTACHED_DEV_TIMEOUT_OPEN_MUIC;
	}
	if (pmuic->opmode & OPMODE_CCIC) {
		if (!mdev_continue_for_TA_USB(pmuic, new_dev)) {
			goto check_vb;
		}
	}
#endif
	if (intr == MUIC_INTR_ATTACH) {
		muic_handle_attach(pmuic, new_dev,
			pmuic->vps.s.adc, pmuic->vps.s.vbvolt);
	} else {
		muic_handle_detach(pmuic);
	}
#if defined(CONFIG_MUIC_SUPPORT_CCIC)	
check_vb:
#endif
#if defined(CONFIG_VBUS_NOTIFIER)
	pr_info("vbus_check %d \n",pmuic->vps.s.vbvolt);
	vbus_notifier_handle(!!pmuic->vps.s.vbvolt ? STATUS_VBUS_HIGH : STATUS_VBUS_LOW);
#endif /* CONFIG_VBUS_NOTIFIER */

}
