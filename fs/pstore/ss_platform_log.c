/* 
 * ss_platform_log.c
 *
 * Copyright (C) 2016 Samsung Electronics
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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_SEC_BSP
#include <linux/sec_bsp.h>
#endif

/* This defines are for PSTORE */
#define SS_LOGGER_LEVEL_HEADER 	(1)
#define SS_LOGGER_LEVEL_PREFIX 	(2)
#define SS_LOGGER_LEVEL_TEXT		(3)
#define SS_LOGGER_LEVEL_MAX		(4)
#define SS_LOGGER_SKIP_COUNT		(4)
#define SS_LOGGER_STRING_PAD		(1)
#define SS_LOGGER_HEADER_SIZE		(68)

#define SS_LOG_ID_MAIN 		(0)
#define SS_LOG_ID_RADIO		(1)
#define SS_LOG_ID_EVENTS		(2)
#define SS_LOG_ID_SYSTEM		(3)
#define SS_LOG_ID_CRASH		(4)
#define SS_LOG_ID_KERNEL		(5)

typedef struct __attribute__((__packed__)) {
	uint8_t magic;
	uint16_t len;
	uint16_t uid;
	uint16_t pid;
} ss_pmsg_log_header_t;

typedef struct __attribute__((__packed__)) {
	unsigned char id;
	uint16_t tid;
	int32_t tv_sec;
	int32_t tv_nsec;
} ss_android_log_header_t;

typedef struct ss_logger {
	uint16_t	len;
	uint16_t	id;
	uint16_t	pid;
	uint16_t	tid;
	uint16_t	uid;
	uint16_t	level;
	int32_t		tv_sec;
	int32_t		tv_nsec;
	char		msg[0];
	char*		buffer;
	void		(*func_hook_logger)(const char*, size_t);
} __attribute__((__packed__)) ss_logger;


static ss_logger logger;

#ifdef CONFIG_SEC_EVENT_LOG
struct event_log_tag_t {
	int nTagNum;
	char *event_msg;
};

enum event_type {
	EVENT_TYPE_INT		= 0,
	EVENT_TYPE_LONG		= 1,
	EVENT_TYPE_STRING	= 2,
	EVENT_TYPE_LIST		= 3,
	EVENT_TYPE_FLOAT 	= 4,
};

// NOTICE : it must have order.
struct event_log_tag_t event_tags[] = {
	{ 42 , "answer"},
	{ 314 , "pi"},
	{ 1003 , "auditd"},
	{ 2718 , "e"},
	{ 2719 , "configuration_changed"},
	{ 2720 , "sync"},
	{ 2721 , "cpu"},
	{ 2722 , "battery_level"},
	{ 2723 , "battery_status"},
	{ 2724 , "power_sleep_requested"},
	{ 2725 , "power_screen_broadcast_send"},
	{ 2726 , "power_screen_broadcast_done"},
	{ 2727 , "power_screen_broadcast_stop"},
	{ 2728 , "power_screen_state"},
	{ 2729 , "power_partial_wake_state"},
	{ 2730 , "battery_discharge"},
	{ 2740 , "location_controller"},
	{ 2741 , "force_gc"},
	{ 2742 , "tickle"},
	{ 2744 , "free_storage_changed"},
	{ 2745 , "low_storage"},
	{ 2746 , "free_storage_left"},
	{ 2747 , "contacts_aggregation"},
	{ 2748 , "cache_file_deleted"},
	{ 2750 , "notification_enqueue"},
	{ 2751 , "notification_cancel"},
	{ 2752 , "notification_cancel_all"},
	{ 2753 , "idle_maintenance_window_start"},
	{ 2754 , "idle_maintenance_window_finish"},
	{ 2755 , "fstrim_start"},
	{ 2756 , "fstrim_finish"},
	{ 2802 , "watchdog"},
	{ 2803 , "watchdog_proc_pss"},
	{ 2804 , "watchdog_soft_reset"},
	{ 2805 , "watchdog_hard_reset"},
	{ 2806 , "watchdog_pss_stats"},
	{ 2807 , "watchdog_proc_stats"},
	{ 2808 , "watchdog_scheduled_reboot"},
	{ 2809 , "watchdog_meminfo"},
	{ 2810 , "watchdog_vmstat"},
	{ 2811 , "watchdog_requested_reboot"},
	{ 2820 , "backup_data_changed"},
	{ 2821 , "backup_start"},
	{ 2822 , "backup_transport_failure"},
	{ 2823 , "backup_agent_failure"},
	{ 2824 , "backup_package"},
	{ 2825 , "backup_success"},
	{ 2826 , "backup_reset"},
	{ 2827 , "backup_initialize"},
	{ 2830 , "restore_start"},
	{ 2831 , "restore_transport_failure"},
	{ 2832 , "restore_agent_failure"},
	{ 2833 , "restore_package"},
	{ 2834 , "restore_success"},
	{ 2840 , "full_backup_package"},
	{ 2841 , "full_backup_agent_failure"},
	{ 2842 , "full_backup_transport_failure"},
	{ 2843 , "full_backup_success"},
	{ 2844 , "full_restore_package"},
	{ 2850 , "backup_transport_lifecycle"},
	{ 3000 , "boot_progress_start"},
	{ 3010 , "boot_progress_system_run"},
	{ 3020 , "boot_progress_preload_start"},
	{ 3030 , "boot_progress_preload_end"},
	{ 3040 , "boot_progress_ams_ready"},
	{ 3050 , "boot_progress_enable_screen"},
	{ 3060 , "boot_progress_pms_start"},
	{ 3070 , "boot_progress_pms_system_scan_start"},
	{ 3080 , "boot_progress_pms_data_scan_start"},
	{ 3090 , "boot_progress_pms_scan_end"},
	{ 3100 , "boot_progress_pms_ready"},
	{ 3110 , "unknown_sources_enabled"},
	{ 3120 , "pm_critical_info"},
	{ 4000 , "calendar_upgrade_receiver"},
	{ 4100 , "contacts_upgrade_receiver"},
	{ 20003 , "dvm_lock_sample"},
	{ 27500 , "notification_panel_revealed"},
	{ 27501 , "notification_panel_hidden"},
	{ 27510 , "notification_visibility_changed"},
	{ 27511 , "notification_expansion"},
	{ 27520 , "notification_clicked"},
	{ 27530 , "notification_canceled"},
	{ 27531 , "notification_visibility" },
	{ 30001 , "am_finish_activity"},
	{ 30002 , "am_task_to_front"},
	{ 30003 , "am_new_intent"},
	{ 30004 , "am_create_task"},
	{ 30005 , "am_create_activity"},
	{ 30006 , "am_restart_activity"},
	{ 30007 , "am_resume_activity"},
	{ 30008 , "am_anr"},
	{ 30009 , "am_activity_launch_time"},
	{ 30010 , "am_proc_bound"},
	{ 30011 , "am_proc_died"},
	{ 30012 , "am_failed_to_pause"},
	{ 30013 , "am_pause_activity"},
	{ 30014 , "am_proc_start"},
	{ 30015 , "am_proc_bad"},
	{ 30016 , "am_proc_good"},
	{ 30017 , "am_low_memory"},
	{ 30018 , "am_destroy_activity"},
	{ 30019 , "am_relaunch_resume_activity"},
	{ 30020 , "am_relaunch_activity"},
	{ 30021 , "am_on_paused_called"},
	{ 30022 , "am_on_resume_called"},
	{ 30023 , "am_kill"},
	{ 30024 , "am_broadcast_discard_filter"},
	{ 30025 , "am_broadcast_discard_app"},
	{ 30030 , "am_create_service"},
	{ 30031 , "am_destroy_service"},
	{ 30032 , "am_process_crashed_too_much"},
	{ 30033 , "am_drop_process"},
	{ 30034 , "am_service_crashed_too_much"},
	{ 30035 , "am_schedule_service_restart"},
	{ 30036 , "am_provider_lost_process"},
	{ 30037 , "am_process_start_timeout"},
	{ 30039 , "am_crash"},
	{ 30040 , "am_wtf"},
	{ 30041 , "am_switch_user"},
	{ 30042 , "am_activity_fully_drawn_time"},
	{ 30043 , "am_focused_activity"},	
	{ 30044 , "am_focused_stack"},
	{ 30045 , "am_pre_boot"},
	{ 30046 , "am_meminfo"},	
	{ 30047 , "am_pss"},	
	{ 30048 , "am_stop_activity"},
	{ 30049 , "am_on_stop_called"},	
	{ 30050 , "am_mem_factor"},	
	{ 31000 , "wm_no_surface_memory"},
	{ 31001 , "wm_task_created"},
	{ 31002 , "wm_task_moved"},
	{ 31003 , "wm_task_removed"},
	{ 31004 , "wm_stack_created"},
	{ 31005 , "wm_home_stack_moved"},
	{ 31006 , "wm_stack_removed"},
	{ 31007 , "boot_progress_enable_screen"},
	{ 32000 , "imf_force_reconnect_ime"},
	{ 36000 , "sysui_statusbar_touch"},
	{ 36001 , "sysui_heads_up_status"},
	{ 36004 , "sysui_status_bar_state"},
	{ 36010 , "sysui_panelbar_touch"},
	{ 36020 , "sysui_notificationpanel_touch"},
	{ 36030 , "sysui_quickpanel_touch"},
	{ 36040 , "sysui_panelholder_touch"},
	{ 36050 , "sysui_searchpanel_touch"},
	{ 40000 , "volume_changed" },
	{ 40001 , "stream_devices_changed" },
	{ 50000 , "menu_item_selected"},
	{ 50001 , "menu_opened"},
	{ 50020 , "connectivity_state_changed"},
	{ 50021 , "wifi_state_changed"},
	{ 50022 , "wifi_event_handled"},
	{ 50023 , "wifi_supplicant_state_changed"},
	{ 50100 , "pdp_bad_dns_address"},
	{ 50101 , "pdp_radio_reset_countdown_triggered"},
	{ 50102 , "pdp_radio_reset"},
	{ 50103 , "pdp_context_reset"},
	{ 50104 , "pdp_reregister_network"},
	{ 50105 , "pdp_setup_fail"},
	{ 50106 , "call_drop"},
	{ 50107 , "data_network_registration_fail"},
	{ 50108 , "data_network_status_on_radio_off"},
	{ 50109 , "pdp_network_drop"},
	{ 50110 , "cdma_data_setup_failed"},
	{ 50111 , "cdma_data_drop"},
	{ 50112 , "gsm_rat_switched"},
	{ 50113 , "gsm_data_state_change"},
	{ 50114 , "gsm_service_state_change"},
	{ 50115 , "cdma_data_state_change"},
	{ 50116 , "cdma_service_state_change"},
	{ 50117 , "bad_ip_address"},
	{ 50118 , "data_stall_recovery_get_data_call_list"},
	{ 50119 , "data_stall_recovery_cleanup"},
	{ 50120 , "data_stall_recovery_reregister"},
	{ 50121 , "data_stall_recovery_radio_restart"},
	{ 50122 , "data_stall_recovery_radio_restart_with_prop"},
	{ 50123 , "gsm_rat_switched_new"},
	{ 50125 , "exp_det_sms_denied_by_user"},
	{ 50128 , "exp_det_sms_sent_by_user"},
	{ 51100 , "netstats_mobile_sample"},
	{ 51101 , "netstats_wifi_sample"},
	{ 51200 , "lockdown_vpn_connecting"},
	{ 51201 , "lockdown_vpn_connected"},
	{ 51202 , "lockdown_vpn_error"},
	{ 51300 , "config_install_failed"},
	{ 51400 , "ifw_intent_matched"},
	{ 52000 , "db_sample"},
	{ 52001 , "http_stats"},
	{ 52002 , "content_query_sample"},
	{ 52003 , "content_update_sample"},
	{ 52004 , "binder_sample"},
	{ 60000 , "viewroot_draw"},
	{ 60001 , "viewroot_layout"},
	{ 60002 , "view_build_drawing_cache"},
	{ 60003 , "view_use_drawing_cache"},
	{ 60100 , "sf_frame_dur"},
	{ 60110 , "sf_stop_bootanim"},
	{ 65537 , "exp_det_netlink_failure"},
	{ 70000 , "screen_toggled"},
	{ 70101 , "browser_zoom_level_change"},
	{ 70102 , "browser_double_tap_duration"},
	{ 70103 , "browser_bookmark_added"},
	{ 70104 , "browser_page_loaded"},
	{ 70105 , "browser_timeonpage"},
	{ 70150 , "browser_snap_center"},
	{ 70151 , "exp_det_attempt_to_call_object_getclass"},
	{ 70200 , "aggregation"},
	{ 70201 , "aggregation_test"},
	{ 70300 , "telephony_event"},
	{ 70301 , "phone_ui_enter"},
	{ 70302 , "phone_ui_exit"},
	{ 70303 , "phone_ui_button_click"},
	{ 70304 , "phone_ui_ringer_query_elapsed"},
	{ 70305 , "phone_ui_multiple_query"},
	{ 70310 , "telecom_event"},
	{ 70311 , "telecom_service"},
	{ 71001 , "qsb_start"},
	{ 71002 , "qsb_click"},
	{ 71003 , "qsb_search"},
	{ 71004 , "qsb_voice_search"},
	{ 71005 , "qsb_exit"},
	{ 71006 , "qsb_latency"},
	{ 73001 , "input_dispatcher_slow_event_processing"},
	{ 73002 , "input_dispatcher_stale_event"},
	{ 73100 , "looper_slow_lap_time"},
	{ 73200 , "choreographer_frame_skip"},
	{ 75000 , "sqlite_mem_alarm_current"},
	{ 75001 , "sqlite_mem_alarm_max"},
	{ 75002 , "sqlite_mem_alarm_alloc_attempt"},
	{ 75003 , "sqlite_mem_released"},
	{ 75004 , "sqlite_db_corrupt"},
	{ 76001 , "tts_speak_success"},
	{ 76002 , "tts_speak_failure"},
	{ 76003 , "tts_v2_speak_success"},
	{ 76004 , "tts_v2_speak_failure"},
	{ 78001 , "exp_det_dispatchCommand_overflow"},
	{ 80100 , "bionic_event_memcpy_buffer_overflow"},
	{ 80105 , "bionic_event_strcat_buffer_overflow"},
	{ 80110 , "bionic_event_memmov_buffer_overflow"},
	{ 80115 , "bionic_event_strncat_buffer_overflow"},
	{ 80120 , "bionic_event_strncpy_buffer_overflow"},
	{ 80125 , "bionic_event_memset_buffer_overflow"},
	{ 80130 , "bionic_event_strcpy_buffer_overflow"},
	{ 80200 , "bionic_event_strcat_integer_overflow"},
	{ 80205 , "bionic_event_strncat_integer_overflow"},
	{ 80300 , "bionic_event_resolver_old_response"},
	{ 80305 , "bionic_event_resolver_wrong_server"},
	{ 80310 , "bionic_event_resolver_wrong_query"},
	{ 90100 , "exp_det_cert_pin_failure"},
	{ 90200 , "lock_screen_type"},
	{ 90201 , "exp_det_device_admin_activated_by_user"},
	{ 90202 , "exp_det_device_admin_declined_by_user"},
	{ 90300 , "install_package_attempt"},
	{ 201001 , "system_update"},
	{ 201002 , "system_update_user"},
	{ 202001 , "vending_reconstruct"},
	{ 202901 , "transaction_event"},
	{ 203001 , "sync_details"},
	{ 203002 , "google_http_request"},
	{ 204001 , "gtalkservice"},
	{ 204002 , "gtalk_connection"},
	{ 204003 , "gtalk_conn_close"},
	{ 204004 , "gtalk_heartbeat_reset"},
	{ 204005 , "c2dm"},
	{ 205001 , "setup_server_timeout"},
	{ 205002 , "setup_required_captcha"},
	{ 205003 , "setup_io_error"},
	{ 205004 , "setup_server_error"},
	{ 205005 , "setup_retries_exhausted"},
	{ 205006 , "setup_no_data_network"},
	{ 205007 , "setup_completed"},
	{ 205008 , "gls_account_tried"},
	{ 205009 , "gls_account_saved"},
	{ 205010 , "gls_authenticate"},
	{ 205011 , "google_mail_switch"},
	{ 206001 , "snet"},
	{ 206003 , "exp_det_snet"},
	{ 1050101 , "nitz_information"},
	{ 1230000 , "am_create_stack"},
	{ 1230001 , "am_remove_stack"},
	{ 1230002 , "am_move_task_to_stack"},
	{ 1230003 , "am_exchange_task_to_stack"},
	{ 1230004 , "am_create_task_to_stack"},
	{ 1230005 , "am_focus_stack"},
	{ 1260001 , "vs_move_task_to_display"},
	{ 1260002 , "vs_create_display"},
	{ 1260003 , "vs_remove_display"},
	{ 1261000 , "am_start_user "},
	{ 1261001 , "am_stop_user "},
	{ 1397638484 , "snet_event_log"},
};

static const char * find_tag_name_from_id ( int id )
{
	int l = 0;
	int r = ARRAY_SIZE(event_tags)-1;
	int mid = 0;
	
	while ( l <= r )
	{		
		mid = (l+r)/2;

		if (event_tags[mid].nTagNum == id )
			return event_tags[mid].event_msg;
		else if ( event_tags[mid].nTagNum < id )
			l = mid + 1;
		else
			r = mid - 1;
	}
	
	return NULL;	
}

static char * parse_buffer(char *buffer, unsigned char type)
{
	unsigned int buf_len =0;
	char buf[64] = {0};
	
	switch(type)
	{
		case EVENT_TYPE_INT:
		{
			int val = *(int *)buffer;
			buffer+=sizeof(int);
			buf_len = snprintf(buf, 64, "%d", val);
			logger.func_hook_logger(buf, buf_len);
		}
		break;
		case EVENT_TYPE_LONG:
		{
			long long val = *(long long *)buffer;
			buffer+=sizeof(long long);
			buf_len = snprintf(buf, 64, "%lld", val);
			logger.func_hook_logger(buf, buf_len);
		}
		break;
		case EVENT_TYPE_FLOAT:
		{
//			float val = *(float  *)buffer;
			buffer+=sizeof(float);
//			buf_len = snprintf(buf, 64, "%f", val);
//			logger.func_hook_logger("log_platform", buf, buf_len);
		}
		break;
		case EVENT_TYPE_STRING:
		{
  			unsigned int len = *(int *)buffer;
  			unsigned int _len = len;
  			if ( len >= 64 ) len = 63;
  			buffer+=sizeof(int);
			memcpy(buf, buffer, len);
 			logger.func_hook_logger(buf, len);
            buffer+=_len;
		}
		break;
	}
	
	return buffer;
}
#endif

static int ss_combine_pmsg(char *buffer, size_t count, unsigned int level)
{
	char *logbuf = logger.buffer;

	if (!logbuf)
		return -ENOMEM;

	switch(level) {
	case SS_LOGGER_LEVEL_HEADER:
		{
			struct tm tmBuf;
			u64 tv_kernel;
			unsigned int logbuf_len;
			unsigned long rem_nsec;
#ifndef CONFIG_SEC_EVENT_LOG
			if (logger.id == SS_LOG_ID_EVENTS)
				break;
#endif
			tv_kernel = local_clock();
			rem_nsec = do_div(tv_kernel, 1000000000);
			time_to_tm(logger.tv_sec, 0, &tmBuf);

			logbuf_len = snprintf(logbuf, SS_LOGGER_HEADER_SIZE,
					"\n[%5lu.%06lu][%d:%16s] %02d-%02d %02d:%02d:%02d.%03d %5d %5d  ",
					(unsigned long)tv_kernel, rem_nsec / 1000,
					raw_smp_processor_id(), current->comm,
					tmBuf.tm_mon + 1, tmBuf.tm_mday,
					tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec,
					logger.tv_nsec / 1000000, logger.pid, logger.tid);

			logger.func_hook_logger(logbuf, logbuf_len - 1);
		}
		break;
	case SS_LOGGER_LEVEL_PREFIX:
		{
			static const char* kPrioChars = "!.VDIWEFS";
			unsigned char prio = logger.msg[0];

			if (logger.id == SS_LOG_ID_EVENTS)
				break;

			logbuf[0] = prio < strlen(kPrioChars) ? kPrioChars[prio] : '?';
			logbuf[1] = ' ';

#ifdef CONFIG_SEC_EVENT_LOG
			logger.msg[0] = 0xff;
#endif

			logger.func_hook_logger(logbuf, SS_LOGGER_LEVEL_PREFIX);
		}
		break;
	case SS_LOGGER_LEVEL_TEXT:
		{
			char *eatnl = buffer + count - SS_LOGGER_STRING_PAD;

			if (logger.id == SS_LOG_ID_EVENTS)
			{
#ifdef CONFIG_SEC_EVENT_LOG
				unsigned int buf_len;
				char buf[64] = {0};
				int tag_id = *(int *)buffer;
				const char * tag_name  = NULL;

				if ( count == 4 && (tag_name = find_tag_name_from_id(tag_id)) != NULL )
				{
					buf_len = snprintf(buf, 64, "# %s ", tag_name);					
					logger.func_hook_logger(buf, buf_len);
				}
				else
				{
					// SINGLE ITEM
					// logger.msg[0] => count == 1 , if event log, it is type.
					if ( logger.msg[0] == EVENT_TYPE_LONG || logger.msg[0] == EVENT_TYPE_INT || logger.msg[0] == EVENT_TYPE_FLOAT )
						parse_buffer(buffer, logger.msg[0]);
					else if ( count > 6 ) // TYPE(1) + ITEMS(1) + SINGLEITEM(5) or STRING(2+4+1>..)
					// CASE 2,3:
					// STRING OR LIST ITEM
					{
						if ( *buffer == EVENT_TYPE_LIST )
						{
							unsigned char items = *(buffer+1);
							unsigned char i = 0;
							buffer+=2;
							
							logger.func_hook_logger("[", 1);
							
							for (;i<items;++i)
							{
								unsigned char type = *buffer;
								buffer++;
								buffer = parse_buffer(buffer, type);
								logger.func_hook_logger(":", 1);
							}
							
							logger.func_hook_logger("]", 1);
		
						}
						else if ( *buffer == EVENT_TYPE_STRING )
							parse_buffer(buffer+1, EVENT_TYPE_STRING);
					}
						
					logger.msg[0]=0xff; // dummy value;	
				}
				break;
#endif
			}
			else
			{
				if (count == SS_LOGGER_SKIP_COUNT && *eatnl != '\0')
					break;

				if (count > 1 && strncmp(buffer, "!@", 2) == 0) {
					pr_info("%s\n", buffer);
#ifdef CONFIG_SEC_BSP
					if (count > 5 && strncmp(buffer, "!@Boot", 6) == 0)
						sec_boot_stat_add(buffer);
#endif
				}
			}
			logger.func_hook_logger(buffer, count - 1);
		}
		break;
	default:
		break;
	}
	return 0;
}

int ss_hook_pmsg(char *buffer, size_t count)
{
	ss_android_log_header_t *header;
	ss_pmsg_log_header_t *pmsg_header;

	if (!logger.buffer)
		return -ENOMEM;

	switch(count) {
	case sizeof(ss_pmsg_log_header_t):
		pmsg_header = (ss_pmsg_log_header_t *)buffer;
		if (pmsg_header->magic != 'l') {
			ss_combine_pmsg(buffer, count, SS_LOGGER_LEVEL_TEXT);
		} else {
			/* save logger data */
			logger.pid = pmsg_header->pid;
			logger.uid = pmsg_header->uid;
			logger.len = pmsg_header->len;
		}
		break;
	case sizeof(ss_android_log_header_t):
		/* save logger data */
		header = (ss_android_log_header_t *)buffer;
		logger.id = header->id;
		logger.tid = header->tid;
		logger.tv_sec = header->tv_sec;
		logger.tv_nsec  = header->tv_nsec;
		if (logger.id > 7) {
			/* write string */
			ss_combine_pmsg(buffer, count, SS_LOGGER_LEVEL_TEXT);
		} else {
			/* write header */
			ss_combine_pmsg(buffer, count, SS_LOGGER_LEVEL_HEADER);
		}
		break;
	case sizeof(unsigned char):
		logger.msg[0] = buffer[0];
		/* write char for prefix */
		ss_combine_pmsg(buffer, count, SS_LOGGER_LEVEL_PREFIX);
		break;
	default:
		/* write string */
		ss_combine_pmsg(buffer, count, SS_LOGGER_LEVEL_TEXT);
		break;
	}

	return 0;
}
EXPORT_SYMBOL(ss_hook_pmsg);

static ulong mem_address;
module_param(mem_address, ulong, S_IRUSR);
MODULE_PARM_DESC(mem_address,
		"start of reserved RAM used to store platform log");

static ulong mem_size;
module_param(mem_size, ulong, S_IRUSR);
MODULE_PARM_DESC(mem_size,
		"size of reserved RAM used to store platform log");

static char *platform_log_buf;
static unsigned platform_log_idx;
static inline void emit_sec_platform_log_char(char c)
{
	platform_log_buf[platform_log_idx % mem_size] = c;
	platform_log_idx++;
}

static inline void ss_hook_logger(const char *buf, size_t size)
{
	size_t i;

	for (i = 0; i < size ; i++)
		emit_sec_platform_log_char(buf[i]);
}

struct ss_plog_platform_data {
	unsigned long mem_size;
	unsigned long mem_address;
};

static int ss_plog_parse_dt(struct platform_device *pdev,
		struct ss_plog_platform_data *pdata)
{
	struct resource *res;

	dev_dbg(&pdev->dev, "using Device Tree\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev,
			"failed to locate DT /reserved-memory resource\n");
		return -EINVAL;
	}

	pdata->mem_size = resource_size(res);
	pdata->mem_address = res->start;

	return 0;
}

static int ss_plog_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ss_plog_platform_data *pdata = dev->platform_data;
	void __iomem *va = 0;
	int err = -EINVAL;

	if (dev->of_node && !pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto fail_out;
		}

		err = ss_plog_parse_dt(pdev, pdata);
		if (err < 0)
			goto fail_out;
	}

	if (!pdata->mem_size) { 
		pr_err("The memory size must be non-zero\n");
		err = -ENOMEM;
		goto fail_out;
	}

	mem_size = pdata->mem_size;
	mem_address = pdata->mem_address;

	pr_info("attached 0x%lx@0x%llx\n",mem_size, (unsigned long long)mem_address);

	platform_set_drvdata(pdev, pdata);

	if(!request_mem_region(mem_address,mem_size,"ss_plog")) {
		pr_err("request mem region (0x%llx@0x%llx) failed\n",
			(unsigned long long)mem_size, (unsigned long long)mem_address);
		goto fail_out;
	}

	va = ioremap_nocache((phys_addr_t)(mem_address), mem_size);
	if (!va) {
		pr_err("Failed to remap plaform log region\n");
		err = -ENOMEM;
		goto fail_out;
	}

	platform_log_buf = va;
	platform_log_idx = 0;
	
	logger.func_hook_logger = ss_hook_logger;
	logger.buffer = vmalloc(PAGE_SIZE * 3);

	if (logger.buffer)
		pr_info("logger buffer alloc address: 0x%p\n", logger.buffer);

	return 0;

fail_out:
	return err;
}

static const struct of_device_id dt_match[] = {
	{ .compatible = "ss_plog" },
	{}
};

static struct platform_driver ss_plog_driver = {
	.probe		= ss_plog_probe,
	.driver		= {
		.name		= "ss_plog",
		.of_match_table	= dt_match,
	},
};

static int __init ss_plog_init(void)
{
	return platform_driver_register(&ss_plog_driver);
}
device_initcall(ss_plog_init);

static void __exit ss_plog_exit(void)
{
	platform_driver_unregister(&ss_plog_driver);
}
module_exit(ss_plog_exit);

