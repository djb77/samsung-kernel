# drivers/staging/samsung/Makefile
#
# Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
#		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=

# Samsung TN Features
obj-$(CONFIG_SEC_EXT)                += sec_log.o
# Samsung sec sysfs Feature
obj-$(CONFIG_SEC_SYSFS)              += sec_sysfs.o
# Samsung Battery
obj-$(CONFIG_BATTERY_SAMSUNG)        += sec_batt.o
# Samsung TN Reset Feature
obj-$(CONFIG_SEC_REBOOT)             += sec_reboot.o

# Samsung TN Debugging Features
obj-$(CONFIG_SEC_DEBUG)              += sec_debug.o
obj-$(CONFIG_SEC_DEBUG)              += sec_debug_test.o
obj-$(CONFIG_SEC_DEBUG)              += sec_debug_hard_reset_hook.o
obj-$(CONFIG_SEC_DEBUG)              += sec_gaf.o
obj-$(CONFIG_SEC_DEBUG_UPLOAD)       += sec_debug_upload.o
obj-$(CONFIG_SEC_DEBUG_RESET_REASON) += sec_debug_reset_reason.o
obj-$(CONFIG_SEC_DEBUG_EXTRA_INFO)	+= sec_debug_extra_info.o
obj-$(CONFIG_SEC_DEBUG_HW_PARAM)	+= sec_debug_hw_param.o
obj-$(CONFIG_SEC_DEBUG_AUTO_SUMMARY)	+= sec_debug_auto_summary.o
obj-$(CONFIG_SEC_DEBUG_LAST_KMSG)	+= sec_debug_last_kmsg.o
obj-$(CONFIG_SEC_DEBUG_PPMPU)		+= sec_debug_ppmpu.o

# Samsung TN BSP Options
obj-$(CONFIG_ARGOS)                  += sec_argos.o
obj-$(CONFIG_SEC_PARAM)		+= sec_param.o
obj-$(CONFIG_SEC_BOOTSTAT)	+= sec_bootstat.o

# Samsung sec NAD Feature
obj-$(CONFIG_SEC_NAD)		+= sec_nad.o

# Samsung TN NAD Balancer Feature
obj-$(CONFIG_SEC_NAD_BALANCER)		+= sec_nad_balancer.o

# Samsung TN Heavy Cpu Count
obj-$(CONFIG_SEC_HEAVY_TASK_CPU)		+= sec_heavy_task_cpu.o

# Samsung Mobile Power Management Debugging Support
obj-$(CONFIG_SEC_PM_DEBUG)		+= sec_pm_debug.o
