#
# Makefile for muic devices
#

obj-y					+= muic-core.o
obj-$(CONFIG_MUIC_NOTIFIER)		+= muic_notifier.o
obj-$(CONFIG_MUIC_MAX77854)		+= max77854-muic.o
obj-$(CONFIG_HV_MUIC_MAX77854_AFC)	+= max77854-muic-afc.o
obj-$(CONFIG_MUIC_MAX77705)		+= max77705-muic.o
obj-$(CONFIG_HV_MUIC_MAX77705_AFC)	+= max77705-muic-afc.o
obj-$(CONFIG_MUIC_MAX77705_CCIC)	+= max77705-muic-ccic.o

# MUIC_UNIVERSAL
ifeq ($(CONFIG_MUIC_UNIVERSAL), y)
  obj-$(CONFIG_MUIC_SUPPORT_CCIC)	+= universal/muic_ccic.o
endif
obj-$(CONFIG_MUIC_UNIVERSAL)	+= universal/muic_usb.o
obj-$(CONFIG_MUIC_UNIVERSAL)	+= universal/muic_coagent.o
#obj-$(CONFIG_MUIC_UNIVERSAL)	+= universal/muic_task.o
obj-$(CONFIG_MUIC_UNIVERSAL)	+= universal/muic_state.o
obj-$(CONFIG_MUIC_UNIVERSAL)	+= universal/muic_apis.o
obj-$(CONFIG_MUIC_UNIVERSAL)	+= universal/muic_sysfs.o
obj-$(CONFIG_MUIC_UNIVERSAL)	+= universal/muic_debug.o
obj-$(CONFIG_MUIC_UNIVERSAL)	+= universal/muic_dt.o
obj-$(CONFIG_MUIC_UNIVERSAL)	+= universal/muic_i2c.o
obj-$(CONFIG_MUIC_UNIVERSAL)	+= universal/muic_regmap.o
obj-$(CONFIG_MUIC_UNIVERSAL)	+= universal/muic_vps.o
obj-$(CONFIG_MUIC_UNIVERSAL_MAX77854)	+= universal/muic_task_max77854.o
obj-$(CONFIG_MUIC_UNIVERSAL_MAX77854)	+= universal/muic_regmap_max77854.o
obj-$(CONFIG_MUIC_HV_MAX77854)		+= universal/muic_hv_max77854.o
obj-$(CONFIG_MUIC_UNIVERSAL_MAX77865)	+= universal/muic_task_max77865.o
obj-$(CONFIG_MUIC_UNIVERSAL_MAX77865)	+= universal/muic_regmap_max77865.o
obj-$(CONFIG_MUIC_HV_MAX77865)		+= universal/muic_hv_max77865.o
