#
# Makefile for ccic devices
#

obj-$(CONFIG_CCIC_NOTIFIER)		+= ccic_notifier.o ccic_sysfs.o
obj-$(CONFIG_CCIC_ALTERNATE_MODE)	+= ccic_alternate.o ccic_misc.o
ifdef CONFIG_SEC_MSM8917_PROJECT
obj-$(CONFIG_CCIC_S2MM005)		+= s2mm005_fw.o s2mm005_cc.o s2mm005x01_pd.o s2mm005.o
else
obj-$(CONFIG_CCIC_S2MM005)		+= s2mm005_fw.o s2mm005_cc.o s2mm005_pd.o s2mm005.o
endif
