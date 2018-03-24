#ifndef __ACPM_H__
#define __ACPM_H__

struct acpm_info {
	unsigned int plugin_num;
	struct device *dev;
	void __iomem *mcore_base;
};

extern void *memcpy_align_4(void *dest, const void *src, unsigned int n);

#define EXYNOS_PMU_CORTEX_APM_CONFIGURATION                           (0x0100)
#define EXYNOS_PMU_CORTEX_APM_STATUS                                  (0x0104)
#define EXYNOS_PMU_CORTEX_APM_OPTION                                  (0x0108)
#define EXYNOS_PMU_CORTEX_APM_DURATION0                               (0x0110)
#define EXYNOS_PMU_CORTEX_APM_DURATION1                               (0x0114)
#define EXYNOS_PMU_CORTEX_APM_DURATION2                               (0x0118)
#define EXYNOS_PMU_CORTEX_APM_DURATION3                               (0x011C)

#define APM_LOCAL_PWR_CFG_RESET         (~(0x1 << 0))

extern struct acpm_framework *acpm_initdata;
extern void __iomem *acpm_srambase;

#endif
