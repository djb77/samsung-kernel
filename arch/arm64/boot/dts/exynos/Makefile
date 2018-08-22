dtb-$(CONFIG_ARCH_EXYNOS) += exynos7-espresso.dtb

dtb-$(CONFIG_SOC_EXYNOS8895) += exynos8895-smdk8895.dtb
dtb-$(CONFIG_SOC_EXYNOS9810) += \
	exynos9810.dtb

dtbo-$(CONFIG_SOC_EXYNOS9810) += \
	exynos9810-universal9810.dtbo \
	exynos9810-universal9810-2.dtbo \
	exynos9810-universal9810-rev02.dtbo \
	exynos9810-universal9810-2-rev02.dtbo

# always		:= $(dtb-y) $(dtbo-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb *.dtbo
