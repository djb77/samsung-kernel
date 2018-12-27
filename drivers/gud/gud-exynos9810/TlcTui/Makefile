#
# Makefile for the Kinibi trusted UI driver
#

GUD_ROOT_FOLDER := drivers/gud/gud-exynos9810
ifeq ($(CONFIG_SOC_EXYNOS9810),y)
PLATFORM := exynos9810
endif

# add our modules to kernel.
obj-$(CONFIG_TRUSTONIC_TRUSTED_UI) += TlcTui.o

TlcTui-y := main.o tlcTui.o trustedui.o \
platforms/$(PLATFORM)/tui-hal.o

# Release mode by default
ccflags-y += -DNDEBUG
ccflags-y += -Wno-declaration-after-statement

ccflags-$(CONFIG_TRUSTONIC_TEE_DEBUG) += -DDEBUG

# MobiCore Driver includes
ccflags-y += -I$(GUD_ROOT_FOLDER)/MobiCoreDriver/public

# MobiCore TlcTui required includes
ccflags-y += -I$(GUD_ROOT_FOLDER)/TlcTui/inc \
	     -I$(GUD_ROOT_FOLDER)/TlcTui/public \
	     -I$(GUD_ROOT_FOLDER)/TlcTui \
	     -Idrivers/video/fbdev/exynos/dpu_9810 \
	     -Idrivers/staging/android/ion/
