#
# Samsung Clock specific Makefile
#

obj-$(CONFIG_SOC_EXYNOS3250)	+= clk-exynos3250.o
obj-$(CONFIG_ARCH_EXYNOS4)	+= clk-exynos4.o
obj-$(CONFIG_SOC_EXYNOS4415)	+= clk-exynos4415.o
obj-$(CONFIG_SOC_EXYNOS5250)	+= clk-exynos5250.o
obj-$(CONFIG_SOC_EXYNOS5260)	+= clk-exynos5260.o
obj-$(CONFIG_SOC_EXYNOS5410)	+= clk-exynos5410.o
obj-$(CONFIG_SOC_EXYNOS5420)	+= clk-exynos5420.o
obj-$(CONFIG_SOC_EXYNOS5433)	+= clk-exynos5433.o
obj-$(CONFIG_SOC_EXYNOS5440)	+= clk-exynos5440.o
ifeq ($(CONFIG_COMPOSITE_CLK_SAMSUNG), y)
	obj-y				+= composite.o
	obj-$(CONFIG_SOC_EXYNOS8890)	+= clk-exynos8890.o
ifneq ($(CONFIG_SOC_EMULATOR8895), y)
		obj-$(CONFIG_SOC_EXYNOS8895)	+= clk-exynos8895.o
else
		obj-$(CONFIG_SOC_EMULATOR8895)	+= clk-emulator8895.o
endif
else
	obj-$(CONFIG_COMMON_CLK)	+= clk.o clk-pll.o clk-cpu.o
endif
obj-$(CONFIG_ARCH_EXYNOS)	+= clk-exynos-audss.o
obj-$(CONFIG_ARCH_EXYNOS)	+= clk-exynos-clkout.o
obj-$(CONFIG_ARCH_EXYNOS7)	+= clk-exynos7.o
obj-$(CONFIG_S3C2410_COMMON_CLK)+= clk-s3c2410.o
obj-$(CONFIG_S3C2410_COMMON_DCLK)+= clk-s3c2410-dclk.o
obj-$(CONFIG_S3C2412_COMMON_CLK)+= clk-s3c2412.o
obj-$(CONFIG_S3C2443_COMMON_CLK)+= clk-s3c2443.o
obj-$(CONFIG_ARCH_S3C64XX)	+= clk-s3c64xx.o
obj-$(CONFIG_ARCH_S5PV210)	+= clk-s5pv210.o clk-s5pv210-audss.o
obj-$(CONFIG_PWM_SAMSUNG)	+= clk-exynos-pwm.o
