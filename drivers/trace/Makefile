#
# Makefile for trace.
#

obj-$(CONFIG_EXYNOS_SNAPSHOT) += exynos-ss.o exynos-ss-log.o exynos-ss-utils.o
obj-$(CONFIG_EXYNOS_CORESIGHT) += exynos-coresight.o
ifeq ($(CONFIG_SOC_EXYNOS9810),y)
obj-$(CONFIG_EXYNOS_ITMON) += exynos9810-itmon.o
endif
obj-$(CONFIG_EXYNOS_CONSOLE_DEBUGGER) += exynos-condbg.o exynos-condbg-dev.o
obj-$(CONFIG_EXYNOS_CORESIGHT_ETM) += exynos-coresight-etm.o
