#
# Makefile for the SCore driver
#

ccflags-$(FPGA_SCORE) += -DSCORE_EVT1
ccflags-$(FPGA_SCORE) += -DCONFIG_FPGA_SCORE
ccflags-$(FPGA_SCORE) += -I./

ccflags-$(STANDALONE_SCORE) += -DCONFIG_STANDALONE_SCORE
ccflags-$(MMIP_SCORE) += -DCONFIG_MMIP_SCORE

ifeq ($(CONFIG_SOC_EXYNOS9810_EVT1),y)
ccflags-$(CONFIG_EXYNOS_SCORE) += -DSCORE_EVT1
else
ccflags-$(CONFIG_EXYNOS_SCORE) += -DSCORE_EVT0
endif
ccflags-$(CONFIG_EXYNOS_SCORE) += -Idrivers/vision/score

score-objs-y += score_device.o
score-objs-y += score_system.o
score-objs-y += score_interface.o
score-objs-y += score_firmware.o
score-objs-y += score_frame.o
score-objs-y += score_print.o
score-objs-y += score_scq.o
score-objs-y += score_sm.o
score-objs-y += score_sr.o
score-objs-y += score_core.o
score-objs-y += score_ioctl.o
ifeq ($(CONFIG_COMPAT),y)
  score-objs-y += score_compat_ioctl.o
endif
score-objs-y += score_context.o
score-objs-y += score_dpmu.o
score-objs-y += score_util.o
score-objs-y += score_profiler.o
score-objs-y += score_mmu.o
score-objs-y += score_sysfs.o

score-objs-$(FPGA_SCORE) += fpga/score_pm.o
score-objs-$(FPGA_SCORE) += fpga/score_clk.o
score-objs-$(FPGA_SCORE) += fpga/score_memory.o

score-objs-$(CONFIG_EXYNOS_SCORE) += exynos/score_pm.o
score-objs-$(CONFIG_EXYNOS_SCORE) += exynos/score_clk.o
score-objs-$(CONFIG_EXYNOS_SCORE) += exynos/score_memory.o
score-objs-$(CONFIG_EXYNOS_SCORE) += exynos/score_ion.o

score-objs := $(score-objs-y)
ifeq ($(FPGA_SCORE),y)
  obj-m += score.o
else
  obj-$(CONFIG_EXYNOS_SCORE) += score.o
endif
