# Makefile of modem_if

EXTRA_CFLAGS += -Idrivers/misc/modem_v1
EXTRA_CFLAGS += -lnet/core/

obj-y += modem_main.o modem_io_device.o
obj-y += modem_net_flowcontrol_device.o modem_utils.o

obj-$(CONFIG_LTE_MODEM_CMC221) += modem_modemctl_device_cmc221.o modem_boot_device_spi.o
obj-$(CONFIG_UMTS_MODEM_SS222) += modem_modemctl_device_ss222.o modem_boot_device_spi.o
obj-$(CONFIG_LTE_MODEM_S5E4270)	+= modem_modemctl_device_sh222ap.o
obj-$(CONFIG_LTE_MODEM_S5E7580) += modem_modemctl_device_sh310ap.o
obj-$(CONFIG_LTE_MODEM_S5E7890) += modem_modemctl_device_sh333ap.o
obj-$(CONFIG_LTE_MODEM_S5E8890) += modem_modemctl_device_sh333ap.o pmu-cp.o

obj-$(CONFIG_LINK_DEVICE_SHMEM)	+= modem_link_device_shmem.o

# Check whether or not memory-type interface
ifeq ($(CONFIG_LINK_DEVICE_SHMEM),y)
LINK_DEVICE_MEMORY_INTERFACE=y
endif

ifdef LINK_DEVICE_MEMORY_INTERFACE
obj-y += modem_link_device_memory.o
endif
