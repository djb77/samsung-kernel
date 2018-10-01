#include <linux/device.h>

#include <linux/notifier.h>
#include <linux/usb/manager/usb_typec_manager_notifier.h>

#include <linux/ccic/ccic_notifier.h>

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/sec_sysfs.h>

#if defined(CONFIG_VBUS_NOTIFIER)
#include <linux/vbus_notifier.h>
#endif
#include <linux/usb_notify.h>

#define MANAGER_WATER_EVENT_ENABLE
#define DEBUG
#define SET_MANAGER_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_MANAGER_NOTIFIER_BLOCK(nb)			\
		SET_MANAGER_NOTIFIER_BLOCK(nb, NULL, -1)

struct device *manager_device;
manager_data_t typec_manager;
#ifdef MANAGER_WATER_EVENT_ENABLE
static DECLARE_COMPLETION(ccic_attach_done);
#endif
void set_usb_enumeration_state(int state);
static void cable_type_check_work(bool state, int time);

static int manager_notifier_notify(void *data)
{
	MANAGER_NOTI_TYPEDEF *manager_noti = (MANAGER_NOTI_TYPEDEF *)data;
	int ret = 0;

	pr_info("usb: [M] %s: src:%s dest:%s id:%s "
		"sub1:%02x sub2:%02x sub3:%02x\n", __func__,
		(manager_noti->src<8)? CCIC_NOTI_DEST_Print[manager_noti->src]:"unknown",
		(manager_noti->dest<8)? CCIC_NOTI_DEST_Print[manager_noti->dest]:"unknown",
		(manager_noti->id<7)? CCIC_NOTI_ID_Print[manager_noti->id]:"unknown",
		manager_noti->sub1, manager_noti->sub2, manager_noti->sub3);	

	if (manager_noti->dest == CCIC_NOTIFY_DEV_USB) {
		if (typec_manager.ccic_drp_state == ((CC_NOTI_USB_STATUS_TYPEDEF *)manager_noti)->drp)
			return 0;
		typec_manager.ccic_drp_state = ((CC_NOTI_USB_STATUS_TYPEDEF *)manager_noti)->drp;
		if (typec_manager.ccic_drp_state == USB_STATUS_NOTIFY_DETACH)
			set_usb_enumeration_state(0);
	}

#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	store_usblog_notify(NOTIFY_MANAGER, (void*)data , NULL);
#endif

	if (manager_noti->dest == CCIC_NOTIFY_DEV_MUIC) {
		ret = blocking_notifier_call_chain(&(typec_manager.manager_muic_notifier),
					manager_noti->id, manager_noti);
	} else {
		ret = blocking_notifier_call_chain(&(typec_manager.manager_ccic_notifier),
					manager_noti->id, manager_noti);
	}

	switch (ret) {
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("usb: [M] %s: notify done(0x%x)\n", __func__, ret);
		break;
	case NOTIFY_STOP_MASK:
	case NOTIFY_BAD:
	default:
		if ( manager_noti->dest == CCIC_NOTIFY_DEV_USB) {
			pr_info("usb: [M] %s: UPSM case (0x%x)\n", __func__, ret);			
			typec_manager.is_UFPS = 1;
		} else {
			pr_info("usb: [M] %s: notify error occur(0x%x)\n", __func__, ret);
		}
		break;
	}

	return ret;
}

void set_usb_enumeration_state(int state)
{
	if(typec_manager.usb_enum_state != state) {
		typec_manager.usb_enum_state = state;

		if(typec_manager.usb_enum_state == 0x310)
			typec_manager.usb310_count++;
		else if(typec_manager.usb_enum_state == 0x210)
			typec_manager.usb210_count++;
	}
}
EXPORT_SYMBOL(set_usb_enumeration_state);

int get_ccic_water_count(void)
{
	int ret;
	ret = typec_manager.water_count;
	typec_manager.water_count = 0;
	return ret;
}
EXPORT_SYMBOL(get_ccic_water_count);

int get_ccic_dry_count(void)
{
	int ret;
	ret = typec_manager.dry_count;
	typec_manager.dry_count = 0;
	return ret;
}
EXPORT_SYMBOL(get_ccic_dry_count);

int get_usb210_count(void)
{
	int ret;
	ret = typec_manager.usb210_count;
	typec_manager.usb210_count = 0;
	return ret;
}
EXPORT_SYMBOL(get_usb210_count);

int get_usb310_count(void)
{
	int ret;
	ret = typec_manager.usb310_count;
	typec_manager.usb310_count = 0;
	return ret;
}
EXPORT_SYMBOL(get_usb310_count);

void set_usb_enable_state(void)
{
	if (!typec_manager.usb_enable_state) {
		typec_manager.usb_enable_state = true;
		if (typec_manager.pd_con_state) {
			cable_type_check_work(true, 120);
		}
	}
}
EXPORT_SYMBOL(set_usb_enable_state);

static void cable_type_check(struct work_struct *work)
{
	CC_NOTI_USB_STATUS_TYPEDEF p_usb_noti;
	CC_NOTI_ATTACH_TYPEDEF p_batt_noti;

	if ( (typec_manager.ccic_drp_state != USB_STATUS_NOTIFY_ATTACH_UFP) ||
		typec_manager.is_UFPS ){
		pr_info("usb: [M] %s: skip case\n", __func__);
		return;
	}

	pr_info("usb: [M] %s: usb=%d, pd=%d\n", __func__, typec_manager.usb_enum_state, typec_manager.pd_con_state);
	if(!typec_manager.usb_enum_state ||
		(typec_manager.muic_data_refresh 
		&& typec_manager.cable_type==MANAGER_NOTIFY_MUIC_CHARGER)) {

		/* TA cable Type */
		p_usb_noti.src = CCIC_NOTIFY_DEV_MANAGER;
		p_usb_noti.dest = CCIC_NOTIFY_DEV_USB;
		p_usb_noti.id = CCIC_NOTIFY_ID_USB;
		p_usb_noti.attach = CCIC_NOTIFY_DETACH;
		p_usb_noti.drp = USB_STATUS_NOTIFY_DETACH;
		p_usb_noti.sub3 = 0;
		p_usb_noti.pd = NULL;
		manager_notifier_notify(&p_usb_noti);

	} else {
		/* USB cable Type */
		p_batt_noti.src = CCIC_NOTIFY_DEV_MANAGER;
		p_batt_noti.dest = CCIC_NOTIFY_DEV_BATTERY;
		p_batt_noti.id = CCIC_NOTIFY_ID_USB;
		p_batt_noti.attach = 0;
		p_batt_noti.rprd = 0;
		p_batt_noti.cable_type = PD_USB_TYPE;
		p_batt_noti.pd = NULL;
		manager_notifier_notify(&p_batt_noti);
	}
}

static void cable_type_check_work(bool state, int time) {
	if(typec_manager.usb_enable_state) {
		cancel_delayed_work_sync(&typec_manager.cable_check_work);
		if(state) {
			schedule_delayed_work(&typec_manager.cable_check_work, msecs_to_jiffies(time*100));
		}
	}
}

static void muic_work_without_ccic(struct work_struct *work)
{
	CC_NOTI_USB_STATUS_TYPEDEF p_usb_noti;

	pr_info("usb: [M] %s: working state=%d, vbus=%s\n", __func__,
		typec_manager.muic_attach_state_without_ccic,
		typec_manager.vbus_state == STATUS_VBUS_HIGH ? "HIGH" : "LOW");

	if (typec_manager.muic_attach_state_without_ccic) {
		switch (typec_manager.muic_action) {
			case MUIC_NOTIFY_CMD_ATTACH:
#if defined(CONFIG_VBUS_NOTIFIER)
				if(typec_manager.vbus_state == STATUS_VBUS_HIGH)
#endif
				{
					p_usb_noti.src = CCIC_NOTIFY_DEV_MUIC;
					p_usb_noti.dest = CCIC_NOTIFY_DEV_USB;
					p_usb_noti.id = CCIC_NOTIFY_ID_USB;
					p_usb_noti.attach = CCIC_NOTIFY_ATTACH;
					p_usb_noti.drp = USB_STATUS_NOTIFY_ATTACH_UFP;
					p_usb_noti.sub3 = 0;
					p_usb_noti.pd = NULL;
					manager_notifier_notify(&p_usb_noti);
				}
				break;
			case MUIC_NOTIFY_CMD_DETACH:
				typec_manager.muic_attach_state_without_ccic = 0;
				p_usb_noti.src = CCIC_NOTIFY_DEV_MUIC;
				p_usb_noti.dest = CCIC_NOTIFY_DEV_USB;
				p_usb_noti.id = CCIC_NOTIFY_ID_USB;
				p_usb_noti.attach = CCIC_NOTIFY_DETACH;
				p_usb_noti.drp = USB_STATUS_NOTIFY_DETACH;
				p_usb_noti.sub3 = 0;
				p_usb_noti.pd = NULL;
				manager_notifier_notify(&p_usb_noti);
				break;
			default :
				break;
		}
	}
}

static int manager_handle_ccic_notification(struct notifier_block *nb,
				unsigned long action, void *data)
{
	MANAGER_NOTI_TYPEDEF *p_noti = (MANAGER_NOTI_TYPEDEF *)data;
	CC_NOTI_ATTACH_TYPEDEF bat_noti;
#ifdef MANAGER_WATER_EVENT_ENABLE
	CC_NOTI_ATTACH_TYPEDEF muic_noti;
#endif
	int ret = 0;

	pr_info("usb: [M] %s: src:%s dest:%s id:%s attach/rid:%d\n", __func__,
		(p_noti->src<8)? CCIC_NOTI_DEST_Print[p_noti->src]:"unknown",
		(p_noti->dest<8)? CCIC_NOTI_DEST_Print[p_noti->dest]:"unknown",
		(p_noti->id<7)? CCIC_NOTI_ID_Print[p_noti->id]:"unknown",
		p_noti->sub1);

	switch (p_noti->id) {
	case CCIC_NOTIFY_ID_POWER_STATUS:
		if(((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach) {
			typec_manager.pd_con_state = 1;	// PDIC_NOTIFY_EVENT_PD_SINK
			if( (typec_manager.ccic_drp_state == USB_STATUS_NOTIFY_ATTACH_UFP) &&
				!typec_manager.is_UFPS){
				pr_info("usb: [M] %s: PD charger + UFP\n", __func__);
				cable_type_check_work(true, 60);
			}
		}
		p_noti->dest = CCIC_NOTIFY_DEV_BATTERY;
		if(typec_manager.pd == NULL)
			typec_manager.pd = p_noti->pd;		
		break;
	case CCIC_NOTIFY_ID_ATTACH:		// for MUIC
			if(typec_manager.ccic_attach_state != ((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach) {
				typec_manager.ccic_attach_state = ((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach;
				typec_manager.muic_data_refresh = 0;
				typec_manager.muic_attach_state_without_ccic = 0;
				typec_manager.is_UFPS = 0;
				if(typec_manager.ccic_attach_state == CCIC_NOTIFY_ATTACH){
					pr_info("usb: [M] %s: CCIC_NOTIFY_ATTACH\n", __func__);
					typec_manager.water_det = 0;
#ifdef MANAGER_WATER_EVENT_ENABLE
					complete(&ccic_attach_done);
#endif
					typec_manager.pd_con_state = 0;
				} else { /* CCIC_NOTIFY_DETACH */
					pr_info("usb: [M] %s: CCIC_NOTIFY_DETACH (pd=%d, cable_type=%d)\n", __func__,
						typec_manager.pd_con_state, typec_manager.cable_type);
					cable_type_check_work(false, 0);
					if(typec_manager.pd_con_state) {
						typec_manager.pd_con_state = 0;
						bat_noti.src = CCIC_NOTIFY_DEV_CCIC;
						bat_noti.dest = CCIC_NOTIFY_DEV_BATTERY;
						bat_noti.id = CCIC_NOTIFY_ID_ATTACH;
						bat_noti.attach = CCIC_NOTIFY_DETACH;
						bat_noti.rprd = 0;
						bat_noti.cable_type = ATTACHED_DEV_UNOFFICIAL_ID_ANY_MUIC; // temp
						bat_noti.pd = NULL;
						manager_notifier_notify(&bat_noti);
					}
				}
			}
		break;
	case CCIC_NOTIFY_ID_RID:	// for MUIC (FAC)
		typec_manager.ccic_rid_state = p_noti->sub1;
		break;
	case CCIC_NOTIFY_ID_USB:	// for USB3
		if (((CC_NOTI_USB_STATUS_TYPEDEF *)p_noti)->drp != USB_STATUS_NOTIFY_DETACH &&
			(typec_manager.ccic_rid_state == RID_523K || typec_manager.ccic_rid_state == RID_619K)) {
			return 0;
		}
		break;
	case CCIC_NOTIFY_ID_WATER:
		if (((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach) {
#ifdef MANAGER_WATER_EVENT_ENABLE
			if(!typec_manager.water_det) {
					typec_manager.water_det = 1;
					typec_manager.water_count++;
					complete(&ccic_attach_done);

					muic_noti.src = CCIC_NOTIFY_DEV_CCIC;
					muic_noti.dest = CCIC_NOTIFY_DEV_MUIC;
					muic_noti.id = CCIC_NOTIFY_ID_WATER;
					muic_noti.attach = CCIC_NOTIFY_ATTACH;
					muic_noti.rprd = 0;
					muic_noti.cable_type = 0;
					muic_noti.pd = NULL;
					manager_notifier_notify(&muic_noti);

					if (typec_manager.muic_action == MUIC_NOTIFY_CMD_ATTACH) {
						p_noti->sub3 = ATTACHED_DEV_UNDEFINED_RANGE_MUIC; /* cable_type */
					} else {
						/* Skip detach case */
						return 0;
					}
			} else {
				/* Ignore duplicate events */
				return 0;
			}
#else
			if(!typec_manager.water_det) {
				typec_manager.water_det = 1;
				if (typec_manager.muic_action == MUIC_NOTIFY_CMD_ATTACH) {
					bat_noti.src = CCIC_NOTIFY_DEV_CCIC;
					bat_noti.dest = CCIC_NOTIFY_DEV_BATTERY;
					bat_noti.id = CCIC_NOTIFY_ID_ATTACH;
					bat_noti.attach = CCIC_NOTIFY_DETACH;
					bat_noti.rprd = 0;
					bat_noti.cable_type = typec_manager.muic_cable_type;
					bat_noti.pd = NULL;
					manager_notifier_notify(&bat_noti);
				}
			}
			return 0;
#endif
		} else {
			typec_manager.water_det = 0;
			typec_manager.dry_count++;
			return 0;
		}
		break;
	default:
		break;
	}	

	ret = manager_notifier_notify(p_noti);

	return ret;	
}

static int manager_handle_muic_notification(struct notifier_block *nb,
				unsigned long action, void *data)
{
	CC_NOTI_ATTACH_TYPEDEF *p_noti = (CC_NOTI_ATTACH_TYPEDEF *)data;
	CC_NOTI_USB_STATUS_TYPEDEF usb_noti;

 	pr_info("usb: [M] %s: attach:%d, cable_type:%d\n", __func__,
		p_noti->attach, p_noti->cable_type);

	typec_manager.muic_action = p_noti->attach;
	typec_manager.muic_cable_type = p_noti->cable_type;
	typec_manager.muic_data_refresh = 1;

	if(typec_manager.water_det){
		/* If Water det irq case is ignored */
		if(p_noti->attach) typec_manager.muic_attach_state_without_ccic = 1;
		pr_info("usb: [M] %s: Water detected case\n", __func__);
		return 0;
	}

	if (p_noti->attach && typec_manager.ccic_attach_state == CCIC_NOTIFY_DETACH) {
		typec_manager.muic_attach_state_without_ccic = 1;
	}

	switch (p_noti->cable_type) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_USB_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		pr_info("usb: [M] %s: USB(%d) %s, CCIC: %s \n", __func__, p_noti->cable_type,
			p_noti->attach ? "Attached": "Detached", typec_manager.ccic_attach_state? "Attached": "Detached");

		if(typec_manager.muic_action) {
			typec_manager.cable_type = MANAGER_NOTIFY_MUIC_USB;
		} else {
			typec_manager.cable_type = MANAGER_NOTIFY_MUIC_NONE;
		}

		if(typec_manager.muic_attach_state_without_ccic) {
			if (p_noti->attach) {
				schedule_delayed_work(&typec_manager.muic_noti_work, msecs_to_jiffies(2000));
			} else {
				schedule_delayed_work(&typec_manager.muic_noti_work, 0);
			}
		}
		break;

	case ATTACHED_DEV_TA_MUIC:
		pr_info("usb: [M] %s: TA(%d) %s \n", __func__, p_noti->cable_type,
			p_noti->attach ? "Attached": "Detached");

		if(typec_manager.muic_action) {
			typec_manager.cable_type = MANAGER_NOTIFY_MUIC_CHARGER;
		} else {
			typec_manager.cable_type = MANAGER_NOTIFY_MUIC_NONE;
		}

		if(p_noti->attach && typec_manager.ccic_drp_state == USB_STATUS_NOTIFY_ATTACH_UFP ) {
			if(typec_manager.pd_con_state) {
				cable_type_check_work(false, 0);
			}	
			/* Turn off the USB Phy when connected to the charger */
			usb_noti.src = CCIC_NOTIFY_DEV_MUIC;
			usb_noti.dest = CCIC_NOTIFY_DEV_USB;
			usb_noti.id = CCIC_NOTIFY_ID_USB;
			usb_noti.attach = CCIC_NOTIFY_DETACH;
			usb_noti.drp = USB_STATUS_NOTIFY_DETACH;
			usb_noti.sub3 = 0;
			usb_noti.pd = NULL;
			manager_notifier_notify(&usb_noti);
		}
		break;

	case ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC:
	case ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC:
		pr_info("usb: [M] %s: AFC or QC Prepare(%d) %s \n", __func__, p_noti->cable_type,
			p_noti->attach ? "Attached": "Detached");
		break;

	default:
		pr_info("usb: [M] %s: Cable(%d) %s \n", __func__, p_noti->cable_type,
			p_noti->attach ? "Attached": "Detached");
		break;
	}

	if(!(p_noti->attach) && typec_manager.pd_con_state) {
		/* If PD charger + detach case is ignored */
		pr_info("usb: [M] %s: PD charger detached case\n", __func__);
	} else {
		p_noti->src = CCIC_NOTIFY_DEV_MUIC;
		p_noti->dest = CCIC_NOTIFY_DEV_BATTERY;
		manager_notifier_notify(p_noti);
	}

	return 0;	
}

#if defined(CONFIG_VBUS_NOTIFIER)
static int manager_handle_vbus_notification(struct notifier_block *nb,
				unsigned long action, void *data)
{
	vbus_status_t vbus_type = *(vbus_status_t *)data;
#ifdef MANAGER_WATER_EVENT_ENABLE
	CC_NOTI_ATTACH_TYPEDEF bat_noti;
#endif
	CC_NOTI_ATTACH_TYPEDEF muic_noti;

	pr_info("usb: [M] %s: cmd=%lu, vbus_type=%s, WATER DET=%d ATTACH=%s (%d)\n", __func__,
		action, vbus_type == STATUS_VBUS_HIGH ? "HIGH" : "LOW", typec_manager.water_det,
		typec_manager.ccic_attach_state == CCIC_NOTIFY_ATTACH ? "ATTACH":"DETATCH",
		typec_manager.muic_attach_state_without_ccic);

	typec_manager.vbus_state = vbus_type;

#ifdef MANAGER_WATER_EVENT_ENABLE
	init_completion(&ccic_attach_done);
	if ((typec_manager.water_det == 1) && (vbus_type == STATUS_VBUS_HIGH) )
		wait_for_completion_timeout(&ccic_attach_done,
					    msecs_to_jiffies(2000));
#endif

	switch (vbus_type) {
	case STATUS_VBUS_HIGH:
#ifdef MANAGER_WATER_EVENT_ENABLE
		if (typec_manager.water_det) {
			bat_noti.src = CCIC_NOTIFY_DEV_CCIC;
			bat_noti.dest = CCIC_NOTIFY_DEV_BATTERY;
			bat_noti.id = CCIC_NOTIFY_ID_WATER;
			bat_noti.attach = CCIC_NOTIFY_ATTACH;
			bat_noti.rprd = 0;
			bat_noti.cable_type = ATTACHED_DEV_UNDEFINED_RANGE_MUIC;
			bat_noti.pd = NULL;
			manager_notifier_notify(&bat_noti);
		}
#endif
		break;
	case STATUS_VBUS_LOW:
#ifdef MANAGER_WATER_EVENT_ENABLE
		if (typec_manager.water_det) {
			bat_noti.src = CCIC_NOTIFY_DEV_CCIC;
			bat_noti.dest = CCIC_NOTIFY_DEV_BATTERY;
			bat_noti.id = CCIC_NOTIFY_ID_ATTACH;
			bat_noti.attach = CCIC_NOTIFY_DETACH;
			bat_noti.rprd = 0;
			bat_noti.cable_type = ATTACHED_DEV_UNDEFINED_RANGE_MUIC;
			bat_noti.pd = NULL;
			manager_notifier_notify(&bat_noti);
		}
#endif
		if (typec_manager.muic_attach_state_without_ccic) {
			muic_noti.src = CCIC_NOTIFY_DEV_MANAGER;
			muic_noti.dest = CCIC_NOTIFY_DEV_MUIC;
			muic_noti.id = CCIC_NOTIFY_ID_ATTACH;
			muic_noti.attach = 0;
			muic_noti.rprd = 0;
			muic_noti.cable_type = typec_manager.muic_cable_type;
			muic_noti.pd = NULL;
			manager_notifier_notify(&muic_noti);
		}
		break;
	default:
		break;
	}

	return 0;
}
#endif

int manager_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			manager_notifier_device_t listener)
{
	int ret = 0;
	MANAGER_NOTI_TYPEDEF m_noti;

	pr_info("usb: [M] %s: listener=%d register\n", __func__, listener);

	/* Check if MANAGER Notifier is ready. */
	if (!manager_device) {
		pr_err("usb: [M] %s: Not Initialized...\n", __func__);
		return -1;
	}

	if (listener == MANAGER_NOTIFY_CCIC_MUIC) {
		SET_MANAGER_NOTIFIER_BLOCK(nb, notifier, listener);
		ret = blocking_notifier_chain_register(&(typec_manager.manager_muic_notifier), nb);
		if (ret < 0)
			pr_err("usb: [M] %s: muic blocking_notifier_chain_register error(%d)\n",
					__func__, ret);
	} else {
		SET_MANAGER_NOTIFIER_BLOCK(nb, notifier, listener);
		ret = blocking_notifier_chain_register(&(typec_manager.manager_ccic_notifier), nb);
		if (ret < 0)
			pr_err("usb: [M] %s: ccic blocking_notifier_chain_register error(%d)\n",
					__func__, ret);
	}

		/* current manager's attached_device status notify */
	if(listener == MANAGER_NOTIFY_CCIC_BATTERY) {
		/* CC_NOTI_ATTACH_TYPEDEF */
		m_noti.src = CCIC_NOTIFY_DEV_MANAGER;
		m_noti.dest = CCIC_NOTIFY_DEV_BATTERY;
		m_noti.sub1 = (typec_manager.ccic_attach_state || typec_manager.muic_action);
		m_noti.sub2 = 0;
		m_noti.pd = typec_manager.pd;
		if(typec_manager.water_det && m_noti.sub1) {
			m_noti.id = CCIC_NOTIFY_ID_WATER;
			m_noti.sub3 = ATTACHED_DEV_UNDEFINED_RANGE_MUIC;
		} else {
			m_noti.id = CCIC_NOTIFY_ID_ATTACH;
			if(typec_manager.pd_con_state) {
				pr_info("usb: [M] %s: PD is attached already\n", __func__);
				m_noti.id = CCIC_NOTIFY_ID_POWER_STATUS;
			} else if(typec_manager.muic_cable_type != ATTACHED_DEV_NONE_MUIC) {
				m_noti.sub3= typec_manager.muic_cable_type;
			} else {
				switch(typec_manager.ccic_drp_state){
					case USB_STATUS_NOTIFY_ATTACH_UFP:
						m_noti.sub3 = ATTACHED_DEV_USB_MUIC;
						break;
					case USB_STATUS_NOTIFY_ATTACH_DFP:
						m_noti.sub3 = ATTACHED_DEV_OTG_MUIC;
						break;
					default:
						m_noti.sub3 = ATTACHED_DEV_NONE_MUIC;
						break;
				}
			}
		}
		pr_info("usb: [M] %s BATTERY: cable_type=%d (%s) \n", __func__, m_noti.sub3,
			typec_manager.muic_cable_type? "MUIC" : "CCIC");
		nb->notifier_call(nb, m_noti.id, &(m_noti));

	} else if(listener == MANAGER_NOTIFY_CCIC_USB) {
		/* CC_NOTI_USB_STATUS_TYPEDEF */
		m_noti.src = CCIC_NOTIFY_DEV_MANAGER;
		m_noti.dest = CCIC_NOTIFY_DEV_USB;
		m_noti.id = CCIC_NOTIFY_ID_USB;
		m_noti.sub1 = typec_manager.ccic_attach_state || typec_manager.muic_action;

		if (m_noti.sub1) {
			if (typec_manager.ccic_drp_state && 
				(typec_manager.cable_type != MANAGER_NOTIFY_MUIC_CHARGER)) {
				m_noti.sub2 = typec_manager.ccic_drp_state;
			} else if (typec_manager.cable_type == MANAGER_NOTIFY_MUIC_USB) {
				typec_manager.ccic_drp_state = USB_STATUS_NOTIFY_ATTACH_UFP;
				m_noti.sub2 = USB_STATUS_NOTIFY_ATTACH_UFP;
			} else {
				typec_manager.ccic_drp_state = USB_STATUS_NOTIFY_DETACH;
				m_noti.sub2 = USB_STATUS_NOTIFY_DETACH;
			}
		} else {
				m_noti.sub2 = USB_STATUS_NOTIFY_DETACH;
		}

		pr_info("usb: [M] %s USB: attach=%d, drp=%s \n", __func__,	m_noti.sub1,
			CCIC_NOTI_USB_STATUS_Print[m_noti.sub2]);
		nb->notifier_call(nb, m_noti.id, &(m_noti));
	}

	return ret;
}

int manager_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("usb: [M] %s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(typec_manager.manager_ccic_notifier), nb);
	if (ret < 0)
		pr_err("usb: [M] %s: ccic blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_MANAGER_NOTIFIER_BLOCK(nb);

	ret = blocking_notifier_chain_unregister(&(typec_manager.manager_muic_notifier), nb);
	if (ret < 0)
		pr_err("usb: [M] %s: muic blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_MANAGER_NOTIFIER_BLOCK(nb);

	return ret;
}

int manager_notifier_init(void)
{
	int ret = 0;

	pr_info("usb: [M] %s\n", __func__);

	manager_device = sec_device_create(NULL, "typec_manager");
	if (IS_ERR(manager_device)) {
		pr_err("usb: [M] %s Failed to create device(switch)!\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	BLOCKING_INIT_NOTIFIER_HEAD(&(typec_manager.manager_ccic_notifier));
	BLOCKING_INIT_NOTIFIER_HEAD(&(typec_manager.manager_muic_notifier));

	// Register manager handler to ccic notifier block list
#if defined(CONFIG_VBUS_NOTIFIER)
	vbus_notifier_register(&typec_manager.vbus_nb, manager_handle_vbus_notification,VBUS_NOTIFY_DEV_MANAGER);
#endif
	ccic_notifier_register(&typec_manager.ccic_nb, manager_handle_ccic_notification,CCIC_NOTIFY_DEV_MANAGER);
	muic_notifier_register(&typec_manager.muic_nb, manager_handle_muic_notification,MUIC_NOTIFY_DEV_MANAGER);
//	usb_notifier_register(&typec_manager.manager_nb, manager_handle_usb_notification,USB_NOTIFY_DEV_MANAGER);
//	battery_notifier_register(&typec_manager.manager_nb, manager_handle_battery_notification,BATTERY_NOTIFY_DEV_MANAGER);

	typec_manager.ccic_attach_state = CCIC_NOTIFY_DETACH;
	typec_manager.ccic_drp_state = USB_STATUS_NOTIFY_DETACH;
	typec_manager.muic_action = MUIC_NOTIFY_CMD_DETACH;
	typec_manager.muic_cable_type = ATTACHED_DEV_NONE_MUIC;
	typec_manager.cable_type = MANAGER_NOTIFY_MUIC_NONE;
	typec_manager.muic_data_refresh = 0;
	typec_manager.usb_enum_state = 0;
	typec_manager.water_det = 0;
	typec_manager.water_count =0;
	typec_manager.dry_count = 0;
	typec_manager.usb210_count = 0;
	typec_manager.usb310_count = 0;
	typec_manager.muic_attach_state_without_ccic = 0;
	typec_manager.vbus_state = 0;
	typec_manager.is_UFPS = 0;
	typec_manager.ccic_rid_state = RID_UNDEFINED;
	typec_manager.pd = NULL;

	INIT_DELAYED_WORK(&typec_manager.cable_check_work,
				  cable_type_check);	

	INIT_DELAYED_WORK(&typec_manager.muic_noti_work,
				  muic_work_without_ccic);	

	pr_info("usb: [M] %s end\n", __func__);
out:
	return ret;
}

