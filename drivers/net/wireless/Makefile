#
# Makefile for the Linux Wireless network device drivers.
#

obj-$(CONFIG_WLAN_VENDOR_ADMTEK) += admtek/
obj-$(CONFIG_WLAN_VENDOR_ATH) += ath/
obj-$(CONFIG_WLAN_VENDOR_ATMEL) += atmel/
obj-$(CONFIG_WLAN_VENDOR_BROADCOM) += broadcom/
obj-$(CONFIG_WLAN_VENDOR_CISCO) += cisco/
obj-$(CONFIG_WLAN_VENDOR_INTEL) += intel/
obj-$(CONFIG_WLAN_VENDOR_INTERSIL) += intersil/
obj-$(CONFIG_WLAN_VENDOR_MARVELL) += marvell/
obj-$(CONFIG_WLAN_VENDOR_MEDIATEK) += mediatek/
obj-$(CONFIG_WLAN_VENDOR_RALINK) += ralink/
obj-$(CONFIG_WLAN_VENDOR_REALTEK) += realtek/
obj-$(CONFIG_WLAN_VENDOR_RSI) += rsi/
obj-$(CONFIG_WLAN_VENDOR_ST) += st/
obj-$(CONFIG_WLAN_VENDOR_TI) += ti/
obj-$(CONFIG_WLAN_VENDOR_ZYDAS) += zydas/

#ifneq ($(CONFIG_BCM4361),)
#obj-$(CONFIG_BCM4361)   += bcmdhd4361/
#obj-$(CONFIG_SOC_EXYNOS9810) += bcmdhd4361/dhd_custom_memprealloc.o
#obj-$(CONFIG_SOC_EXYNOS9810) += bcmdhd4361/dhd_custom_exynos.o
#endif

# Android Version Check from Platform source
ifneq ($(ANDROID_VERSION),)
MAJOR_VERSION := $(shell echo $(ANDROID_VERSION) | rev | cut -c 5- | rev)
DHDCFLAGS += -DANDROID_PLATFORM_VERSION=$(MAJOR_VERSION)
$(warning android version 1: $(ANDROID_VERSION))
$(warning MAJOR_VERSION version 1: $(MAJOR_VERSION))
else
ifneq ($(PLATFORM_VERSION),)
MAJOR_VERSION := $(shell echo $(PLATFORM_VERSION) | cut -d "." -f 1)
DHDCFLAGS += -DANDROID_PLATFORM_VERSION=$(MAJOR_VERSION)
$(warning platform version 2 : $(PLATFORM_VERSION))
$(warning MAJOR_VERSION version 2: $(MAJOR_VERSION))
endif
endif
ifneq ($(CONFIG_BCM4361),)
ifeq ($(shell expr $(MAJOR_VERSION) \>= 10),1)
$(warning platform version 3 : $(PLATFORM_VERSION))
$(warning MAJOR_VERSION version 3: $(MAJOR_VERSION))
obj-$(CONFIG_BCM4361)   += bcmdhd_100_15/
obj-$(CONFIG_SOC_EXYNOS9810) += bcmdhd_100_15/dhd_custom_memprealloc.o
obj-$(CONFIG_SOC_EXYNOS9810) += bcmdhd_100_15/dhd_custom_exynos.o
else
$(warning platform version 4 : $(PLATFORM_VERSION))
$(warning MAJOR_VERSION version 4: $(MAJOR_VERSION))
obj-$(CONFIG_BCM4361)   += bcmdhd_100_10/
obj-$(CONFIG_SOC_EXYNOS9810) += bcmdhd_100_10/dhd_custom_memprealloc.o
obj-$(CONFIG_SOC_EXYNOS9810) += bcmdhd_100_10/dhd_custom_exynos.o
endif
endif


# 16-bit wireless PCMCIA client drivers
obj-$(CONFIG_PCMCIA_RAYCS)	+= ray_cs.o
obj-$(CONFIG_PCMCIA_WL3501)	+= wl3501_cs.o

obj-$(CONFIG_USB_NET_RNDIS_WLAN)	+= rndis_wlan.o

obj-$(CONFIG_MAC80211_HWSIM)	+= mac80211_hwsim.o
