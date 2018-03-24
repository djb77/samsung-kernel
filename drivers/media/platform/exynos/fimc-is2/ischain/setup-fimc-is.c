/* linux/arch/arm/mach-exynos/setup-fimc-is.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * FIMC-IS gpio and clock configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif

#include <exynos-fimc-is.h>

struct platform_device; /* don't need the contents */

/*------------------------------------------------------*/
/*		Common control				*/
/*------------------------------------------------------*/

#define PRINT_CLK(c, n) printk(KERN_DEBUG "[@] %s : 0x%08X\n", n, readl(c));
#define REGISTER_CLK(name) {name, NULL}

#if defined(CONFIG_SOC_EXYNOS8895)
struct fimc_is_clk fimc_is_clk_list[] = {
	REGISTER_CLK("GATE_ISP_EWGEN_ISPHQ"),
	REGISTER_CLK("GATE_IS_ISPHQ_3AA"),
	REGISTER_CLK("GATE_IS_ISPHQ_ISPHQ"),
	REGISTER_CLK("GATE_IS_ISPHQ_QE_3AA"),
	REGISTER_CLK("GATE_IS_ISPHQ_QE_ISPHQ"),
	REGISTER_CLK("GATE_ISPHQ_CMU_ISPHQ"),
	REGISTER_CLK("GATE_PMU_ISPHQ"),
	REGISTER_CLK("GATE_SYSREG_ISPHQ"),
	REGISTER_CLK("UMUX_CLKCMU_ISPHQ_BUS"),

	REGISTER_CLK("GATE_ISP_EWGEN_ISPLP"),
	REGISTER_CLK("GATE_IS_ISPLP_3AAW"),
	REGISTER_CLK("GATE_IS_ISPLP_ISPLP"),
	REGISTER_CLK("GATE_IS_ISPLP_QE_3AAW"),
	REGISTER_CLK("GATE_IS_ISPLP_QE_ISPLP"),
	REGISTER_CLK("GATE_IS_ISPLP_SMMU_ISPLP"),
	REGISTER_CLK("GATE_IS_ISPLP_BCM_ISPLP"),
	REGISTER_CLK("GATE_BTM_ISPLP"),
	REGISTER_CLK("GATE_ISPLP_CMU_ISPLP"),
	REGISTER_CLK("GATE_PMU_ISPLP"),
	REGISTER_CLK("GATE_SYSREG_ISPLP"),
	REGISTER_CLK("UMUX_CLKCMU_ISPLP_BUS"),

	REGISTER_CLK("GATE_ISP_EWGEN_CAM"),
	REGISTER_CLK("GATE_IS_CAM_CSIS0"),
	REGISTER_CLK("GATE_IS_CAM_CSIS1"),
	REGISTER_CLK("GATE_IS_CAM_CSIS2"),
	REGISTER_CLK("GATE_IS_CAM_CSIS3"),
	REGISTER_CLK("GATE_IS_CAM_MC_SCALER"),
	REGISTER_CLK("GATE_IS_CAM_CSISX4_DMA"),
	REGISTER_CLK("GATE_IS_CAM_SYSMMU_CAM0"),
	REGISTER_CLK("GATE_IS_CAM_SYSMMU_CAM1"),
	REGISTER_CLK("GATE_IS_CAM_BCM_CAM0"),
	REGISTER_CLK("GATE_IS_CAM_BCM_CAM1"),
	REGISTER_CLK("GATE_IS_CAM_TPU0"),
	REGISTER_CLK("GATE_IS_CAM_VRA"),
	REGISTER_CLK("GATE_IS_CAM_QE_TPU0"),
	REGISTER_CLK("GATE_IS_CAM_QE_VRA"),
	REGISTER_CLK("GATE_IS_CAM_BNS"),
	REGISTER_CLK("GATE_IS_CAM_QE_CSISX4"),
	REGISTER_CLK("GATE_IS_CAM_QE_TPU1"),
	REGISTER_CLK("GATE_IS_CAM_TPU1"),
	REGISTER_CLK("GATE_BTM_CAMD0"),
	REGISTER_CLK("GATE_BTM_CAMD1"),
	REGISTER_CLK("GATE_CAM_CMU_CAM"),
	REGISTER_CLK("GATE_PMU_CAM"),
	REGISTER_CLK("GATE_SYSREG_CAM"),
	REGISTER_CLK("UMUX_CLKCMU_CAM_BUS"),
	REGISTER_CLK("UMUX_CLKCMU_CAM_TPU0"),
	REGISTER_CLK("UMUX_CLKCMU_CAM_VRA"),
	REGISTER_CLK("UMUX_CLKCMU_CAM_TPU1"),

	REGISTER_CLK("GATE_DCP"),
	REGISTER_CLK("GATE_BTM_DCAM"),
	REGISTER_CLK("GATE_DCAM_CMU_DCAM"),
	REGISTER_CLK("GATE_PMU_DCAM"),
	REGISTER_CLK("GATE_BCM_DCAM"),
	REGISTER_CLK("GATE_SYSREG_DCAM"),
	REGISTER_CLK("UMUX_CLKCMU_DCAM_BUS"),
	REGISTER_CLK("UMUX_CLKCMU_DCAM_IMGD"),

	REGISTER_CLK("GATE_SRDZ"),
	REGISTER_CLK("GATE_BTM_SRDZ"),
	REGISTER_CLK("GATE_PMU_SRDZ"),
	REGISTER_CLK("GATE_BCM_SRDZ"),
	REGISTER_CLK("GATE_SMMU_SRDZ"),
	REGISTER_CLK("GATE_SRDZ_CMU_SRDZ"),
	REGISTER_CLK("GATE_SYSREG_SRDZ"),
	REGISTER_CLK("UMUX_CLKCMU_SRDZ_BUS"),
	REGISTER_CLK("UMUX_CLKCMU_SRDZ_IMGD"),

	REGISTER_CLK("MUX_CIS_CLK0"),
	REGISTER_CLK("MUX_CIS_CLK1"),
	REGISTER_CLK("MUX_CIS_CLK2"),
	REGISTER_CLK("MUX_CIS_CLK3"),

	REGISTER_CLK("CIS_CLK0"),
	REGISTER_CLK("CIS_CLK1"),
	REGISTER_CLK("CIS_CLK2"),
	REGISTER_CLK("CIS_CLK3"),
};

#elif defined(CONFIG_SOC_EXYNOS8890)
struct fimc_is_clk fimc_is_clk_list[] = {
	REGISTER_CLK("oscclk"),
	REGISTER_CLK("gate_fimc_isp0"),
	REGISTER_CLK("gate_fimc_tpu"),
	REGISTER_CLK("isp0"),
	REGISTER_CLK("isp0_tpu"),
	REGISTER_CLK("isp0_trex"),
	REGISTER_CLK("pxmxdx_isp0_pxl"),
	REGISTER_CLK("gate_fimc_isp1"),
	REGISTER_CLK("isp1"),
	REGISTER_CLK("isp_sensor0"),
	REGISTER_CLK("isp_sensor1"),
	REGISTER_CLK("isp_sensor2"),
	REGISTER_CLK("isp_sensor3"),
	REGISTER_CLK("gate_csis0"),
	REGISTER_CLK("gate_csis1"),
	REGISTER_CLK("gate_fimc_bns"),
	REGISTER_CLK("gate_fimc_3aa0"),
	REGISTER_CLK("gate_fimc_3aa1"),
	REGISTER_CLK("gate_hpm"),
	REGISTER_CLK("pxmxdx_csis0"),
	REGISTER_CLK("pxmxdx_csis1"),
	REGISTER_CLK("pxmxdx_csis2"),
	REGISTER_CLK("pxmxdx_csis3"),
	REGISTER_CLK("pxmxdx_3aa0"),
	REGISTER_CLK("pxmxdx_3aa1"),
	REGISTER_CLK("pxmxdx_trex"),
	REGISTER_CLK("hs0_csis0_rx_byte"),
	REGISTER_CLK("hs1_csis0_rx_byte"),
	REGISTER_CLK("hs2_csis0_rx_byte"),
	REGISTER_CLK("hs3_csis0_rx_byte"),
	REGISTER_CLK("hs0_csis1_rx_byte"),
	REGISTER_CLK("hs1_csis1_rx_byte"),
	REGISTER_CLK("gate_isp_cpu"),
	REGISTER_CLK("gate_csis2"),
	REGISTER_CLK("gate_csis3"),
	REGISTER_CLK("gate_fimc_vra"),
	REGISTER_CLK("gate_mc_scaler"),
	REGISTER_CLK("gate_i2c0_isp"),
	REGISTER_CLK("gate_i2c1_isp"),
	REGISTER_CLK("gate_i2c2_isp"),
	REGISTER_CLK("gate_i2c3_isp"),
	REGISTER_CLK("gate_wdt_isp"),
	REGISTER_CLK("gate_mcuctl_isp"),
	REGISTER_CLK("gate_uart_isp"),
	REGISTER_CLK("gate_pdma_isp"),
	REGISTER_CLK("gate_pwm_isp"),
	REGISTER_CLK("gate_spi0_isp"),
	REGISTER_CLK("gate_spi1_isp"),
	REGISTER_CLK("isp_spi0"),
	REGISTER_CLK("isp_spi1"),
	REGISTER_CLK("isp_uart"),
	REGISTER_CLK("gate_sclk_pwm_isp"),
	REGISTER_CLK("gate_sclk_uart_isp"),
	REGISTER_CLK("cam1_arm"),
	REGISTER_CLK("cam1_vra"),
	REGISTER_CLK("cam1_trex"),
	REGISTER_CLK("cam1_bus"),
	REGISTER_CLK("cam1_peri"),
	REGISTER_CLK("cam1_csis2"),
	REGISTER_CLK("cam1_csis3"),
	REGISTER_CLK("cam1_scl"),
	REGISTER_CLK("cam1_phy0_csis2"),
	REGISTER_CLK("cam1_phy1_csis2"),
	REGISTER_CLK("cam1_phy2_csis2"),
	REGISTER_CLK("cam1_phy3_csis2"),
	REGISTER_CLK("cam1_phy0_csis3"),
	REGISTER_CLK("mipi_dphy_m4s4"),
};

#elif defined(CONFIG_SOC_EXYNOS7570)
struct fimc_is_clk fimc_is_clk_list[] = {
	REGISTER_CLK("oscclk"),
	REGISTER_CLK("isp_ppmu"),
	REGISTER_CLK("isp_bts"),
	REGISTER_CLK("isp_cam"),
	REGISTER_CLK("isp_vra"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user"),
	REGISTER_CLK("gate_mscl"),
	REGISTER_CLK("mif_isp_sensor0"),
};

#elif defined(CONFIG_SOC_EXYNOS7870)
struct fimc_is_clk fimc_is_clk_list[] = {
	REGISTER_CLK("oscclk"),
	REGISTER_CLK("gate_isp_sysmmu"),
	REGISTER_CLK("gate_isp_ppmu"),
	REGISTER_CLK("gate_isp_bts"),
	REGISTER_CLK("gate_isp_cam"),
	REGISTER_CLK("gate_isp_isp"),
	REGISTER_CLK("gate_isp_vra"),
	REGISTER_CLK("pxmxdx_isp_isp"),
	REGISTER_CLK("pxmxdx_isp_cam"),
	REGISTER_CLK("pxmxdx_isp_vra"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user"),
	REGISTER_CLK("isp_sensor0_sclk"),
	REGISTER_CLK("isp_sensor1_sclk"),
	REGISTER_CLK("isp_sensor2_sclk"),
};

#elif defined(CONFIG_SOC_EXYNOS7880)
struct fimc_is_clk fimc_is_clk_list[] = {
	REGISTER_CLK("oscclk"),
	REGISTER_CLK("gate_isp_sysmmu"),
	REGISTER_CLK("gate_isp_ppmu"),
	REGISTER_CLK("gate_isp_bts"),
	REGISTER_CLK("gate_isp_cam"),
	REGISTER_CLK("gate_isp_isp"),
	REGISTER_CLK("gate_isp_vra"),
	REGISTER_CLK("pxmxdx_isp_isp"),
	REGISTER_CLK("pxmxdx_isp_cam"),
	REGISTER_CLK("pxmxdx_isp_vra"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs1_s4_user"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs2_s4_user"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs3_s4_user"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs1_s4s_user"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs2_s4s_user"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs3_s4s_user"),
	REGISTER_CLK("isp_sensor0_sclk"),
	REGISTER_CLK("isp_sensor1_sclk"),
	REGISTER_CLK("isp_sensor2_sclk"),
	REGISTER_CLK("hsi2c_frontcam"),
	REGISTER_CLK("hsi2c_maincam"),
	REGISTER_CLK("hsi2c_depthcam"),
	REGISTER_CLK("hsi2c_frontsensor"),
	REGISTER_CLK("hsi2c_rearaf"),
	REGISTER_CLK("hsi2c_rearsensor"),
	REGISTER_CLK("spi_rearfrom"),
	REGISTER_CLK("spi_frontfrom"),
	REGISTER_CLK("sclk_spi_frontfrom"),
	REGISTER_CLK("sclk_spi_rearfrom"),
};
#else
struct fimc_is_clk fimc_is_clk_list[] = {
	REGISTER_CLK("null")
};
#endif

int fimc_is_set_rate(struct device *dev,
	const char *name, ulong frequency)
{
	int ret = 0;
	size_t index;
	struct clk *clk = NULL;

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); index++) {
		if (!strcmp(name, fimc_is_clk_list[index].name))
			clk = fimc_is_clk_list[index].clk;
	}

	if (IS_ERR_OR_NULL(clk)) {
		pr_err("[@][ERR] %s: clk_target_list is NULL : %s\n", __func__, name);
		return -EINVAL;
	}

	ret = clk_set_rate(clk, frequency);
	if (ret) {
		pr_err("[@][ERR] %s: clk_set_rate is fail(%s)\n", __func__, name);
		return ret;
	}

	/* fimc_is_get_rate_dt(dev, name); */

	return ret;
}

/* utility function to get rate with DT */
ulong fimc_is_get_rate(struct device *dev,
	const char *name)
{
	ulong frequency;
	size_t index;
	struct clk *clk = NULL;

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); index++) {
		if (!strcmp(name, fimc_is_clk_list[index].name))
			clk = fimc_is_clk_list[index].clk;
	}

	if (IS_ERR_OR_NULL(clk)) {
		pr_err("[@][ERR] %s: clk_target_list is NULL : %s\n", __func__, name);
		return -EINVAL;
	}

	frequency = clk_get_rate(clk);

#ifdef DBG_DUMPCMU
	pr_info("[@] %s : %ldMhz (enable_count : %d)\n", name, frequency/1000000, __clk_get_enable_count(clk));
#endif

	return frequency;
}

/* utility function to eable with DT */
int  fimc_is_enable(struct device *dev,
	const char *name)
{
	int ret = 0;
	size_t index;
	struct clk *clk = NULL;

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); index++) {
		if (!strcmp(name, fimc_is_clk_list[index].name))
			clk = fimc_is_clk_list[index].clk;
	}

	if (IS_ERR_OR_NULL(clk)) {
		pr_err("[@][ERR] %s: clk_target_list is NULL : %s\n", __func__, name);
		return -EINVAL;
	}

#ifdef DBG_DUMPCMU
	pr_info("[@][ENABLE] %s : (enable_count : %d)\n", name, __clk_get_enable_count(clk));
#endif

	ret = clk_prepare_enable(clk);
	if (ret) {
		pr_err("[@][ERR] %s: clk_prepare_enable is fail(%s)\n", __func__, name);
		return ret;
	}

	return ret;
}

/* utility function to disable with DT */
int fimc_is_disable(struct device *dev,
	const char *name)
{
	size_t index;
	struct clk *clk = NULL;

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); index++) {
		if (!strcmp(name, fimc_is_clk_list[index].name))
			clk = fimc_is_clk_list[index].clk;
	}

	if (IS_ERR_OR_NULL(clk)) {
		pr_err("[@][ERR] %s: clk_target_list is NULL : %s\n", __func__, name);
		return -EINVAL;
	}

	clk_disable_unprepare(clk);

#ifdef DBG_DUMPCMU
	pr_info("[@][DISABLE] %s : (enable_count : %d)\n", name, __clk_get_enable_count(clk));
#endif

	return 0;
}

/* utility function to set parent with DT */
int fimc_is_set_parent_dt(struct device *dev,
	const char *child, const char *parent)
{
	int ret = 0;
	struct clk *p;
	struct clk *c;

	p = clk_get(dev, parent);
	if (IS_ERR_OR_NULL(p)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, parent);
		return -EINVAL;
	}

	c = clk_get(dev, child);
	if (IS_ERR_OR_NULL(c)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, child);
		return -EINVAL;
	}

	ret = clk_set_parent(c, p);
	if (ret) {
		pr_err("%s: clk_set_parent is fail(%s -> %s)\n", __func__, child, parent);
		return ret;
	}

	return 0;
}

/* utility function to set rate with DT */
int fimc_is_set_rate_dt(struct device *dev,
	const char *conid, unsigned int rate)
{
	int ret = 0;
	struct clk *target;

	target = clk_get(dev, conid);
	if (IS_ERR_OR_NULL(target)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, conid);
		return -EINVAL;
	}

	ret = clk_set_rate(target, rate);
	if (ret) {
		pr_err("%s: clk_set_rate is fail(%s)\n", __func__, conid);
		return ret;
	}

	/* fimc_is_get_rate_dt(dev, conid); */

	return 0;
}

/* utility function to get rate with DT */
ulong fimc_is_get_rate_dt(struct device *dev,
	const char *conid)
{
	struct clk *target;
	ulong rate_target;

	target = clk_get(dev, conid);
	if (IS_ERR_OR_NULL(target)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, conid);
		return -EINVAL;
	}

	rate_target = clk_get_rate(target);
	pr_info("[@] %s : %ldMhz\n", conid, rate_target/1000000);

	return rate_target;
}

/* utility function to eable with DT */
int fimc_is_enable_dt(struct device *dev,
	const char *conid)
{
	int ret;
	struct clk *target;

	target = clk_get(dev, conid);
	if (IS_ERR_OR_NULL(target)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, conid);
		return -EINVAL;
	}

	ret = clk_prepare(target);
	if (ret) {
		pr_err("%s: clk_prepare is fail(%s)\n", __func__, conid);
		return ret;
	}

	ret = clk_enable(target);
	if (ret) {
		pr_err("%s: clk_enable is fail(%s)\n", __func__, conid);
		return ret;
	}

	return 0;
}

/* utility function to disable with DT */
int fimc_is_disable_dt(struct device *dev,
	const char *conid)
{
	struct clk *target;

	target = clk_get(dev, conid);
	if (IS_ERR_OR_NULL(target)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, conid);
		return -EINVAL;
	}

	clk_disable(target);
	clk_unprepare(target);

	return 0;
}

#if defined(CONFIG_SOC_EXYNOS8895)
int exynos8895_fimc_is_print_clk(struct device *dev);
static void exynos8895_fimc_is_print_clk_reg(void)
{

}

int exynos8895_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
	return 0;
}

int exynos8895_fimc_is_uart_gate(struct device *dev, bool mask)
{
	return 0;
}

int exynos8895_fimc_is_get_clk(struct device *dev)
{
	struct clk *clk;
	int i;

	for (i = 0; i < ARRAY_SIZE(fimc_is_clk_list); i++) {
		clk = devm_clk_get(dev, fimc_is_clk_list[i].name);
		if (IS_ERR_OR_NULL(clk)) {
			pr_err("[@][ERR] %s: could not lookup clock : %s\n",
				__func__, fimc_is_clk_list[i].name);
			return -EINVAL;
		}

		fimc_is_clk_list[i].clk = clk;
	}

	return 0;
}

int exynos8895_fimc_is_cfg_clk(struct device *dev)
{
	return 0;
}

int exynos8895_fimc_is_clk_on(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (pdata->clock_on) {
		ret = pdata->clk_off(dev);
		if (ret) {
			pr_err("clk_off is fail(%d)\n", ret);
			goto p_err;
		}
	}

	fimc_is_enable(dev, "GATE_ISP_EWGEN_ISPHQ");
	fimc_is_enable(dev, "GATE_IS_ISPHQ_3AA");
	fimc_is_enable(dev, "GATE_IS_ISPHQ_ISPHQ");
	fimc_is_enable(dev, "GATE_IS_ISPHQ_QE_3AA");
	fimc_is_enable(dev, "GATE_IS_ISPHQ_QE_ISPHQ");
	fimc_is_enable(dev, "GATE_ISPHQ_CMU_ISPHQ");
	fimc_is_enable(dev, "GATE_PMU_ISPHQ");
	fimc_is_enable(dev, "GATE_SYSREG_ISPHQ");
	fimc_is_enable(dev, "UMUX_CLKCMU_ISPHQ_BUS");

	fimc_is_enable(dev, "GATE_ISP_EWGEN_ISPLP");
	fimc_is_enable(dev, "GATE_IS_ISPLP_3AAW");
	fimc_is_enable(dev, "GATE_IS_ISPLP_ISPLP");
	fimc_is_enable(dev, "GATE_IS_ISPLP_QE_3AAW");
	fimc_is_enable(dev, "GATE_IS_ISPLP_QE_ISPLP");
	fimc_is_enable(dev, "GATE_IS_ISPLP_SMMU_ISPLP");
	fimc_is_enable(dev, "GATE_IS_ISPLP_BCM_ISPLP");
	fimc_is_enable(dev, "GATE_BTM_ISPLP");
	fimc_is_enable(dev, "GATE_ISPLP_CMU_ISPLP");
	fimc_is_enable(dev, "GATE_PMU_ISPLP");
	fimc_is_enable(dev, "GATE_SYSREG_ISPLP");
	fimc_is_enable(dev, "UMUX_CLKCMU_ISPLP_BUS");

	fimc_is_enable(dev, "GATE_ISP_EWGEN_CAM");
	fimc_is_enable(dev, "GATE_IS_CAM_CSIS0");
	fimc_is_enable(dev, "GATE_IS_CAM_CSIS1");
	fimc_is_enable(dev, "GATE_IS_CAM_CSIS2");
	fimc_is_enable(dev, "GATE_IS_CAM_CSIS3");
	fimc_is_enable(dev, "GATE_IS_CAM_MC_SCALER");
	fimc_is_enable(dev, "GATE_IS_CAM_CSISX4_DMA");
	fimc_is_enable(dev, "GATE_IS_CAM_SYSMMU_CAM0");
	fimc_is_enable(dev, "GATE_IS_CAM_SYSMMU_CAM1");
	fimc_is_enable(dev, "GATE_IS_CAM_BCM_CAM0");
	fimc_is_enable(dev, "GATE_IS_CAM_BCM_CAM1");
	fimc_is_enable(dev, "GATE_IS_CAM_TPU0");
	fimc_is_enable(dev, "GATE_IS_CAM_VRA");
	fimc_is_enable(dev, "GATE_IS_CAM_QE_TPU0");
	fimc_is_enable(dev, "GATE_IS_CAM_QE_VRA");
	fimc_is_enable(dev, "GATE_IS_CAM_BNS");
	fimc_is_enable(dev, "GATE_IS_CAM_QE_CSISX4");
	fimc_is_enable(dev, "GATE_IS_CAM_QE_TPU1");
	fimc_is_enable(dev, "GATE_IS_CAM_TPU1");
	fimc_is_enable(dev, "GATE_BTM_CAMD0");
	fimc_is_enable(dev, "GATE_BTM_CAMD1");
	fimc_is_enable(dev, "GATE_CAM_CMU_CAM");
	fimc_is_enable(dev, "GATE_PMU_CAM");
	fimc_is_enable(dev, "GATE_SYSREG_CAM");
	fimc_is_enable(dev, "UMUX_CLKCMU_CAM_BUS");
	fimc_is_enable(dev, "UMUX_CLKCMU_CAM_TPU0");
	fimc_is_enable(dev, "UMUX_CLKCMU_CAM_VRA");
	fimc_is_enable(dev, "UMUX_CLKCMU_CAM_TPU1");

	fimc_is_enable(dev, "GATE_DCP");
	fimc_is_enable(dev, "GATE_BTM_DCAM");
	fimc_is_enable(dev, "GATE_DCAM_CMU_DCAM");
	fimc_is_enable(dev, "GATE_PMU_DCAM");
	fimc_is_enable(dev, "GATE_BCM_DCAM");
	fimc_is_enable(dev, "GATE_SYSREG_DCAM");
	fimc_is_enable(dev, "UMUX_CLKCMU_DCAM_BUS");
	fimc_is_enable(dev, "UMUX_CLKCMU_DCAM_IMGD");

	fimc_is_enable(dev, "GATE_SRDZ");
	fimc_is_enable(dev, "GATE_BTM_SRDZ");
	fimc_is_enable(dev, "GATE_PMU_SRDZ");
	fimc_is_enable(dev, "GATE_BCM_SRDZ");
	fimc_is_enable(dev, "GATE_SMMU_SRDZ");
	fimc_is_enable(dev, "GATE_SRDZ_CMU_SRDZ");
	fimc_is_enable(dev, "GATE_SYSREG_SRDZ");
	fimc_is_enable(dev, "UMUX_CLKCMU_SRDZ_BUS");
	fimc_is_enable(dev, "UMUX_CLKCMU_SRDZ_IMGD");

	fimc_is_enable(dev, "MUX_CIS_CLK0");
	fimc_is_enable(dev, "MUX_CIS_CLK1");
	fimc_is_enable(dev, "MUX_CIS_CLK2");
	fimc_is_enable(dev, "MUX_CIS_CLK3");

	fimc_is_enable(dev, "CIS_CLK0");
	fimc_is_enable(dev, "CIS_CLK1");
	fimc_is_enable(dev, "CIS_CLK2");
	fimc_is_enable(dev, "CIS_CLK3");

#ifdef DBG_DUMPCMU
	exynos8895_fimc_is_print_clk(dev);
#endif

	pdata->clock_on = true;

p_err:
	return 0;
}

int exynos8895_fimc_is_clk_off(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (!pdata->clock_on) {
		pr_err("clk_off is fail(already off)\n");
		ret = -EINVAL;
		goto p_err;
	}

	fimc_is_disable(dev, "GATE_ISP_EWGEN_ISPHQ");
	fimc_is_disable(dev, "GATE_IS_ISPHQ_3AA");
	fimc_is_disable(dev, "GATE_IS_ISPHQ_ISPHQ");
	fimc_is_disable(dev, "GATE_IS_ISPHQ_QE_3AA");
	fimc_is_disable(dev, "GATE_IS_ISPHQ_QE_ISPHQ");
	fimc_is_disable(dev, "GATE_ISPHQ_CMU_ISPHQ");
	fimc_is_disable(dev, "GATE_PMU_ISPHQ");
	fimc_is_disable(dev, "GATE_SYSREG_ISPHQ");
	fimc_is_disable(dev, "UMUX_CLKCMU_ISPHQ_BUS");

	fimc_is_disable(dev, "GATE_ISP_EWGEN_ISPLP");
	fimc_is_disable(dev, "GATE_IS_ISPLP_3AAW");
	fimc_is_disable(dev, "GATE_IS_ISPLP_ISPLP");
	fimc_is_disable(dev, "GATE_IS_ISPLP_QE_3AAW");
	fimc_is_disable(dev, "GATE_IS_ISPLP_QE_ISPLP");
	fimc_is_disable(dev, "GATE_IS_ISPLP_SMMU_ISPLP");
	fimc_is_disable(dev, "GATE_IS_ISPLP_BCM_ISPLP");
	fimc_is_disable(dev, "GATE_BTM_ISPLP");
	fimc_is_disable(dev, "GATE_ISPLP_CMU_ISPLP");
	fimc_is_disable(dev, "GATE_PMU_ISPLP");
	fimc_is_disable(dev, "GATE_SYSREG_ISPLP");
	fimc_is_disable(dev, "UMUX_CLKCMU_ISPLP_BUS");

	fimc_is_disable(dev, "GATE_ISP_EWGEN_CAM");
	fimc_is_disable(dev, "GATE_IS_CAM_CSIS0");
	fimc_is_disable(dev, "GATE_IS_CAM_CSIS1");
	fimc_is_disable(dev, "GATE_IS_CAM_CSIS2");
	fimc_is_disable(dev, "GATE_IS_CAM_CSIS3");
	fimc_is_disable(dev, "GATE_IS_CAM_MC_SCALER");
	fimc_is_disable(dev, "GATE_IS_CAM_CSISX4_DMA");
	fimc_is_disable(dev, "GATE_IS_CAM_SYSMMU_CAM0");
	fimc_is_disable(dev, "GATE_IS_CAM_SYSMMU_CAM1");
	fimc_is_disable(dev, "GATE_IS_CAM_BCM_CAM0");
	fimc_is_disable(dev, "GATE_IS_CAM_BCM_CAM1");
	fimc_is_disable(dev, "GATE_IS_CAM_TPU0");
	fimc_is_disable(dev, "GATE_IS_CAM_VRA");
	fimc_is_disable(dev, "GATE_IS_CAM_QE_TPU0");
	fimc_is_disable(dev, "GATE_IS_CAM_QE_VRA");
	fimc_is_disable(dev, "GATE_IS_CAM_BNS");
	fimc_is_disable(dev, "GATE_IS_CAM_QE_CSISX4");
	fimc_is_disable(dev, "GATE_IS_CAM_QE_TPU1");
	fimc_is_disable(dev, "GATE_IS_CAM_TPU1");
	fimc_is_disable(dev, "GATE_BTM_CAMD0");
	fimc_is_disable(dev, "GATE_BTM_CAMD1");
	fimc_is_disable(dev, "GATE_CAM_CMU_CAM");
	fimc_is_disable(dev, "GATE_PMU_CAM");
	fimc_is_disable(dev, "GATE_SYSREG_CAM");
	fimc_is_disable(dev, "UMUX_CLKCMU_CAM_BUS");
	fimc_is_disable(dev, "UMUX_CLKCMU_CAM_TPU0");
	fimc_is_disable(dev, "UMUX_CLKCMU_CAM_VRA");
	fimc_is_disable(dev, "UMUX_CLKCMU_CAM_TPU1");

	fimc_is_disable(dev, "GATE_DCP");
	fimc_is_disable(dev, "GATE_BTM_DCAM");
	fimc_is_disable(dev, "GATE_DCAM_CMU_DCAM");
	fimc_is_disable(dev, "GATE_PMU_DCAM");
	fimc_is_disable(dev, "GATE_BCM_DCAM");
	fimc_is_disable(dev, "GATE_SYSREG_DCAM");
	fimc_is_disable(dev, "UMUX_CLKCMU_DCAM_BUS");
	fimc_is_disable(dev, "UMUX_CLKCMU_DCAM_IMGD");

	fimc_is_disable(dev, "GATE_SRDZ");
	fimc_is_disable(dev, "GATE_BTM_SRDZ");
	fimc_is_disable(dev, "GATE_PMU_SRDZ");
	fimc_is_disable(dev, "GATE_BCM_SRDZ");
	fimc_is_disable(dev, "GATE_SMMU_SRDZ");
	fimc_is_disable(dev, "GATE_SRDZ_CMU_SRDZ");
	fimc_is_disable(dev, "GATE_SYSREG_SRDZ");
	fimc_is_disable(dev, "UMUX_CLKCMU_SRDZ_BUS");
	fimc_is_disable(dev, "UMUX_CLKCMU_SRDZ_IMGD");

	fimc_is_disable(dev, "MUX_CIS_CLK0");
	fimc_is_disable(dev, "MUX_CIS_CLK1");
	fimc_is_disable(dev, "MUX_CIS_CLK2");
	fimc_is_disable(dev, "MUX_CIS_CLK3");

	fimc_is_disable(dev, "CIS_CLK0");
	fimc_is_disable(dev, "CIS_CLK1");
	fimc_is_disable(dev, "CIS_CLK2");
	fimc_is_disable(dev, "CIS_CLK3");

	pdata->clock_on = false;

p_err:
	return 0;
}

int exynos8895_fimc_is_print_clk(struct device *dev)
{
	fimc_is_get_rate_dt(dev, "GATE_ISP_EWGEN_ISPHQ");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPHQ_3AA");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPHQ_ISPHQ");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPHQ_QE_3AA");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPHQ_QE_ISPHQ");
	fimc_is_get_rate_dt(dev, "GATE_ISPHQ_CMU_ISPHQ");
	fimc_is_get_rate_dt(dev, "GATE_PMU_ISPHQ");
	fimc_is_get_rate_dt(dev, "GATE_SYSREG_ISPHQ");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_ISPHQ_BUS");

	fimc_is_get_rate_dt(dev, "GATE_ISP_EWGEN_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_3AAW");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_QE_3AAW");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_QE_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_SMMU_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_BCM_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_BTM_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_ISPLP_CMU_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_PMU_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_SYSREG_ISPLP");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_ISPLP_BUS");

	fimc_is_get_rate_dt(dev, "GATE_ISP_EWGEN_CAM");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_CSIS0");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_CSIS1");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_CSIS2");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_CSIS3");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_MC_SCALER");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_CSISX4_DMA");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_SYSMMU_CAM0");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_SYSMMU_CAM1");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_BCM_CAM0");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_BCM_CAM1");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_TPU0");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_VRA");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_QE_TPU0");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_QE_VRA");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_BNS");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_QE_CSISX4");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_QE_TPU1");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_TPU1");
	fimc_is_get_rate_dt(dev, "GATE_BTM_CAMD0");
	fimc_is_get_rate_dt(dev, "GATE_BTM_CAMD1");
	fimc_is_get_rate_dt(dev, "GATE_CAM_CMU_CAM");
	fimc_is_get_rate_dt(dev, "GATE_PMU_CAM");
	fimc_is_get_rate_dt(dev, "GATE_SYSREG_CAM");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_CAM_BUS");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_CAM_TPU0");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_CAM_VRA");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_CAM_TPU1");

	fimc_is_get_rate_dt(dev, "GATE_DCP");
	fimc_is_get_rate_dt(dev, "GATE_BTM_DCAM");
	fimc_is_get_rate_dt(dev, "GATE_DCAM_CMU_DCAM");
	fimc_is_get_rate_dt(dev, "GATE_PMU_DCAM");
	fimc_is_get_rate_dt(dev, "GATE_BCM_DCAM");
	fimc_is_get_rate_dt(dev, "GATE_SYSREG_DCAM");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_DCAM_BUS");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_DCAM_IMGD");

	fimc_is_get_rate_dt(dev, "GATE_SRDZ");
	fimc_is_get_rate_dt(dev, "GATE_BTM_SRDZ");
	fimc_is_get_rate_dt(dev, "GATE_PMU_SRDZ");
	fimc_is_get_rate_dt(dev, "GATE_BCM_SRDZ");
	fimc_is_get_rate_dt(dev, "GATE_SMMU_SRDZ");
	fimc_is_get_rate_dt(dev, "GATE_SRDZ_CMU_SRDZ");
	fimc_is_get_rate_dt(dev, "GATE_SYSREG_SRDZ");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_SRDZ_BUS");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_SRDZ_IMGD");

	fimc_is_get_rate_dt(dev, "MUX_CIS_CLK0");
	fimc_is_get_rate_dt(dev, "MUX_CIS_CLK1");
	fimc_is_get_rate_dt(dev, "MUX_CIS_CLK2");
	fimc_is_get_rate_dt(dev, "MUX_CIS_CLK3");

	fimc_is_get_rate_dt(dev, "CIS_CLK0");
	fimc_is_get_rate_dt(dev, "CIS_CLK1");
	fimc_is_get_rate_dt(dev, "CIS_CLK2");
	fimc_is_get_rate_dt(dev, "CIS_CLK3");

	exynos8895_fimc_is_print_clk_reg();

	return 0;
}

#elif defined(CONFIG_SOC_EXYNOS8890)
/* for debug */
void __iomem *cmu_top;
void __iomem *cmu_cam0;
void __iomem *cmu_cam1;
void __iomem *cmu_isp0;
void __iomem *cmu_isp1;

int exynos8890_fimc_is_print_clk(struct device *dev);
static void exynos8890_fimc_is_print_clk_reg(void)
{
	printk(KERN_DEBUG "[@] TOP0\n");
	PRINT_CLK((cmu_top + 0x0100), "BUS0_PLL_CON0");
	PRINT_CLK((cmu_top + 0x0120), "BUS1_PLL_CON0");
	PRINT_CLK((cmu_top + 0x0140), "BUS2_PLL_CON0");
	PRINT_CLK((cmu_top + 0x0160), "BUS3_PLL_CON0");
	PRINT_CLK((cmu_top + 0x01A0), "ISP_PLL_CON0");
	PRINT_CLK((cmu_top + 0x0180), "MFC_PLL_CON0");
	PRINT_CLK((cmu_top + 0x0200), "CLK_CON_MUX_BUS0_PLL");
	PRINT_CLK((cmu_top + 0x0204), "CLK_CON_MUX_BUS1_PLL");
	PRINT_CLK((cmu_top + 0x0208), "CLK_CON_MUX_BUS2_PLL");
	PRINT_CLK((cmu_top + 0x020C), "CLK_CON_MUX_BUS3_PLL");
	PRINT_CLK((cmu_top + 0x0210), "CLK_CON_MUX_MFC_PLL");
	PRINT_CLK((cmu_top + 0x0214), "CLK_CON_MUX_ISP_PLL");
	PRINT_CLK((cmu_top + 0x0220), "CLK_CON_MUX_SCLK_BUS0_PLL");
	PRINT_CLK((cmu_top + 0x0224), "CLK_CON_MUX_SCLK_BUS1_PLL");
	PRINT_CLK((cmu_top + 0x0228), "CLK_CON_MUX_SCLK_BUS2_PLL");
	PRINT_CLK((cmu_top + 0x022C), "CLK_CON_MUX_SCLK_BUS3_PLL");
	PRINT_CLK((cmu_top + 0x0230), "CLK_CON_MUX_SCLK_MFC_PLL");
	PRINT_CLK((cmu_top + 0x0234), "CLK_CON_MUX_SCLK_ISP_PLL");
	PRINT_CLK((cmu_top + 0x02B8), "CLK_CON_MUX_ACLK_CAM0_CSIS0_414");
	PRINT_CLK((cmu_top + 0x02BC), "CLK_CON_MUX_ACLK_CAM0_CSIS1_168");
	PRINT_CLK((cmu_top + 0x02C0), "CLK_CON_MUX_ACLK_CAM0_CSIS2_234");
	PRINT_CLK((cmu_top + 0x02C4), "CLK_CON_MUX_ACLK_CAM0_3AA0_414");
	PRINT_CLK((cmu_top + 0x02C8), "CLK_CON_MUX_ACLK_CAM0_3AA1_414");
	PRINT_CLK((cmu_top + 0x02CC), "CLK_CON_MUX_ACLK_CAM0_CSIS3_132");
	PRINT_CLK((cmu_top + 0x02D0), "CLK_CON_MUX_ACLK_CAM0_TREX_528");
	PRINT_CLK((cmu_top + 0x02D4), "CLK_CON_MUX_ACLK_CAM1_ARM_672");
	PRINT_CLK((cmu_top + 0x02D8), "CLK_CON_MUX_ACLK_CAM1_TREX_VRA_528");
	PRINT_CLK((cmu_top + 0x02DC), "CLK_CON_MUX_ACLK_CAM1_TREX_B_528");
	PRINT_CLK((cmu_top + 0x02E0), "CLK_CON_MUX_ACLK_CAM1_BUS_264");
	PRINT_CLK((cmu_top + 0x02E4), "CLK_CON_MUX_ACLK_CAM1_PERI_84");
	PRINT_CLK((cmu_top + 0x02E8), "CLK_CON_MUX_ACLK_CAM1_CSIS2_414");
	PRINT_CLK((cmu_top + 0x02EC), "CLK_CON_MUX_ACLK_CAM1_CSIS3_132");
	PRINT_CLK((cmu_top + 0x02F0), "CLK_CON_MUX_ACLK_CAM1_SCL_566");
	PRINT_CLK((cmu_top + 0x02A8), "CLK_CON_MUX_ACLK_ISP0_ISP0_528");
	PRINT_CLK((cmu_top + 0x02AC), "CLK_CON_MUX_ACLK_ISP0_TPU_400");
	PRINT_CLK((cmu_top + 0x02B0), "CLK_CON_MUX_ACLK_ISP0_TREX_528");
	PRINT_CLK((cmu_top + 0x02B4), "CLK_CON_MUX_ACLK_ISP1_ISP1_468");
	PRINT_CLK((cmu_top + 0x0368), "CLK_CON_MUX_SCLK_CAM1_ISP_SPI0");
	PRINT_CLK((cmu_top + 0x036C), "CLK_CON_MUX_SCLK_CAM1_ISP_SPI1");
	PRINT_CLK((cmu_top + 0x0370), "CLK_CON_MUX_SCLK_CAM1_ISP_UART");
	PRINT_CLK((cmu_top + 0x0B00), "CLK_CON_MUX_SCLK_ISP_SENSOR0");
	PRINT_CLK((cmu_top + 0x0B10), "CLK_CON_MUX_SCLK_ISP_SENSOR1");
	PRINT_CLK((cmu_top + 0x0B20), "CLK_CON_MUX_SCLK_ISP_SENSOR2");
	PRINT_CLK((cmu_top + 0x0B30), "CLK_CON_MUX_SCLK_ISP_SENSOR3");
	PRINT_CLK((cmu_top + 0x0418), "CLK_CON_DIV_ACLK_CAM0_CSIS0_414");
	PRINT_CLK((cmu_top + 0x041C), "CLK_CON_DIV_ACLK_CAM0_CSIS1_168");
	PRINT_CLK((cmu_top + 0x0420), "CLK_CON_DIV_ACLK_CAM0_CSIS2_234");
	PRINT_CLK((cmu_top + 0x0424), "CLK_CON_DIV_ACLK_CAM0_3AA0_414");
	PRINT_CLK((cmu_top + 0x0428), "CLK_CON_DIV_ACLK_CAM0_3AA1_414");
	PRINT_CLK((cmu_top + 0x042C), "CLK_CON_DIV_ACLK_CAM0_CSIS3_132");
	PRINT_CLK((cmu_top + 0x0430), "CLK_CON_DIV_ACLK_CAM0_TREX_528");
	PRINT_CLK((cmu_top + 0x0434), "CLK_CON_DIV_ACLK_CAM1_ARM_672");
	PRINT_CLK((cmu_top + 0x0438), "CLK_CON_DIV_ACLK_CAM1_TREX_VRA_528");
	PRINT_CLK((cmu_top + 0x043C), "CLK_CON_DIV_ACLK_CAM1_TREX_B_528");
	PRINT_CLK((cmu_top + 0x0440), "CLK_CON_DIV_ACLK_CAM1_BUS_264");
	PRINT_CLK((cmu_top + 0x0444), "CLK_CON_DIV_ACLK_CAM1_PERI_84");
	PRINT_CLK((cmu_top + 0x0448), "CLK_CON_DIV_ACLK_CAM1_CSIS2_414");
	PRINT_CLK((cmu_top + 0x044C), "CLK_CON_DIV_ACLK_CAM1_CSIS3_132");
	PRINT_CLK((cmu_top + 0x0450), "CLK_CON_DIV_ACLK_CAM1_SCL_566");
	PRINT_CLK((cmu_top + 0x0408), "CLK_CON_DIV_ACLK_ISP0_ISP0_528");
	PRINT_CLK((cmu_top + 0x040C), "CLK_CON_DIV_ACLK_ISP0_TPU_400");
	PRINT_CLK((cmu_top + 0x0410), "CLK_CON_DIV_ACLK_ISP0_TREX_528");
	PRINT_CLK((cmu_top + 0x0414), "CLK_CON_DIV_ACLK_ISP1_ISP1_468");
	PRINT_CLK((cmu_top + 0x04C8), "CLK_CON_DIV_SCLK_CAM1_ISP_SPI0");
	PRINT_CLK((cmu_top + 0x04CC), "CLK_CON_DIV_SCLK_CAM1_ISP_SPI1");
	PRINT_CLK((cmu_top + 0x04D0), "CLK_CON_DIV_SCLK_CAM1_ISP_UART");
	PRINT_CLK((cmu_top + 0x0B04), "CLK_CON_DIV_SCLK_ISP_SENSOR0");
	PRINT_CLK((cmu_top + 0x0B14), "CLK_CON_DIV_SCLK_ISP_SENSOR1");
	PRINT_CLK((cmu_top + 0x0B24), "CLK_CON_DIV_SCLK_ISP_SENSOR2");
	PRINT_CLK((cmu_top + 0x0B34), "CLK_CON_DIV_SCLK_ISP_SENSOR3");
	PRINT_CLK((cmu_top + 0x0878), "CLK_ENABLE_ACLK_CAM0_CSIS1_414");
	PRINT_CLK((cmu_top + 0x087C), "CLK_ENABLE_ACLK_CAM0_CSIS1_168");
	PRINT_CLK((cmu_top + 0x0880), "CLK_ENABLE_ACLK_CAM0_CSIS2_234");
	PRINT_CLK((cmu_top + 0x0884), "CLK_ENABLE_ACLK_CAM0_3AA0_414");
	PRINT_CLK((cmu_top + 0x0888), "CLK_ENABLE_ACLK_CAM0_3AA1_414");
	PRINT_CLK((cmu_top + 0x088C), "CLK_ENABLE_ACLK_CAM0_CSIS3_132");
	PRINT_CLK((cmu_top + 0x0890), "CLK_ENABLE_ACLK_CAM0_TREX_528");
	PRINT_CLK((cmu_top + 0x0894), "CLK_ENABLE_ACLK_CAM1_ARM_672");
	PRINT_CLK((cmu_top + 0x0898), "CLK_ENABLE_ACLK_CAM1_TREX_VRA_528");
	PRINT_CLK((cmu_top + 0x089C), "CLK_ENABLE_ACLK_CAM1_TREX_B_528");
	PRINT_CLK((cmu_top + 0x08A0), "CLK_ENABLE_ACLK_CAM1_BUS_264");
	PRINT_CLK((cmu_top + 0x08A4), "CLK_ENABLE_ACLK_CAM1_PERI_84");
	PRINT_CLK((cmu_top + 0x08A8), "CLK_ENABLE_ACLK_CAM1_CSIS2_414");
	PRINT_CLK((cmu_top + 0x08AC), "CLK_ENABLE_ACLK_CAM1_CSIS3_132");
	PRINT_CLK((cmu_top + 0x08B0), "CLK_ENABLE_ACLK_CAM1_SCL_566");
	PRINT_CLK((cmu_top + 0x0868), "CLK_ENABLE_ACLK_ISP0_ISP0_528");
	PRINT_CLK((cmu_top + 0x086C), "CLK_ENABLE_ACLK_ISP0_TPU_400");
	PRINT_CLK((cmu_top + 0x0870), "CLK_ENABLE_ACLK_ISP0_TREX_528");
	PRINT_CLK((cmu_top + 0x0874), "CLK_ENABLE_ACLK_ISP1_ISP1_468");
	PRINT_CLK((cmu_top + 0x0974), "CLK_ENABLE_SCLK_CAM1_ISP_SPI0");
	PRINT_CLK((cmu_top + 0x0978), "CLK_ENABLE_SCLK_CAM1_ISP_SPI1");
	PRINT_CLK((cmu_top + 0x097C), "CLK_ENABLE_SCLK_CAM1_ISP_UART");
	PRINT_CLK((cmu_top + 0x0B0C), "CLK_ENABLE_SCLK_ISP_SENSOR0");
	PRINT_CLK((cmu_top + 0x0B1C), "CLK_ENABLE_SCLK_ISP_SENSOR1");
	PRINT_CLK((cmu_top + 0x0B2C), "CLK_ENABLE_SCLK_ISP_SENSOR2");
	PRINT_CLK((cmu_top + 0x0B3C), "CLK_ENABLE_SCLK_ISP_SENSOR3");

	printk(KERN_DEBUG "[@] CAM0\n");
	PRINT_CLK((cmu_cam0 +  0x0200), "CLK_CON_MUX_ACLK_CAM0_CSIS0_414_USER");
	PRINT_CLK((cmu_cam0 +  0x0204), "CLK_CON_MUX_ACLK_CAM0_CSIS1_168_USER");
	PRINT_CLK((cmu_cam0 +  0x0208), "CLK_CON_MUX_ACLK_CAM0_CSIS2_234_USER");
	PRINT_CLK((cmu_cam0 +  0x020C), "CLK_CON_MUX_ACLK_CAM0_CSIS3_132_USER");
	PRINT_CLK((cmu_cam0 +  0x0214), "CLK_CON_MUX_ACLK_CAM0_3AA0_414_USER");
	PRINT_CLK((cmu_cam0 +  0x0218), "CLK_CON_MUX_ACLK_CAM0_3AA1_414_USER");
	PRINT_CLK((cmu_cam0 +  0x021C), "CLK_CON_MUX_ACLK_CAM0_TREX_528_USER");
	PRINT_CLK((cmu_cam0 +  0x0220), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS0_CSIS0_USER");
	PRINT_CLK((cmu_cam0 +  0x0224), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS1_CSIS0_USER");
	PRINT_CLK((cmu_cam0 +  0x0228), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS2_CSIS0_USER");
	PRINT_CLK((cmu_cam0 +  0x022C), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS3_CSIS0_USER");
	PRINT_CLK((cmu_cam0 +  0x0230), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS0_CSIS1_USER");
	PRINT_CLK((cmu_cam0 +  0x0234), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS1_CSIS1_USER");
	PRINT_CLK((cmu_cam0 +  0x0400), "CLK_CON_DIV_PCLK_CAM0_CSIS0_207");
	PRINT_CLK((cmu_cam0 +  0x040C), "CLK_CON_DIV_PCLK_CAM0_3AA0_207");
	PRINT_CLK((cmu_cam0 +  0x0410), "CLK_CON_DIV_PCLK_CAM0_3AA1_207");
	PRINT_CLK((cmu_cam0 +  0x0414), "CLK_CON_DIV_PCLK_CAM0_TREX_264");
	PRINT_CLK((cmu_cam0 +  0x0418), "CLK_CON_DIV_PCLK_CAM0_TREX_132");
	PRINT_CLK((cmu_cam0 +  0x0800), "CLK_ENABLE_ACLK_CAM0_CSIS0_414");
	PRINT_CLK((cmu_cam0 +  0x0804), "CLK_ENABLE_PCLK_CAM0_CSIS0_207");
	PRINT_CLK((cmu_cam0 +  0x080C), "CLK_ENABLE_ACLK_CAM0_CSIS1_168");
	PRINT_CLK((cmu_cam0 +  0x0818), "CLK_ENABLE_ACLK_CAM0_CSIS2_234");
	PRINT_CLK((cmu_cam0 +  0x081C), "CLK_ENABLE_ACLK_CAM0_CSIS3_132");
	PRINT_CLK((cmu_cam0 +  0x0828), "CLK_ENABLE_ACLK_CAM0_3AA0_414");
	PRINT_CLK((cmu_cam0 +  0x082C), "CLK_ENABLE_PCLK_CAM0_3AA0_207");
	PRINT_CLK((cmu_cam0 +  0x0830), "CLK_ENABLE_ACLK_CAM0_3AA1_414");
	PRINT_CLK((cmu_cam0 +  0x0834), "CLK_ENABLE_PCLK_CAM0_3AA1_207");
	PRINT_CLK((cmu_cam0 +  0x0838), "CLK_ENABLE_ACLK_CAM0_TREX_528");
	PRINT_CLK((cmu_cam0 +  0x083C), "CLK_ENABLE_PCLK_CAM0_TREX_264");
	PRINT_CLK((cmu_cam0 +  0x0840), "CLK_ENABLE_PCLK_CAM0_TREX_132");
	PRINT_CLK((cmu_cam0 +  0x0848), "CLK_ENABLE_PHYCLK_HS0_CSIS0_RX_BYTE");
	PRINT_CLK((cmu_cam0 +  0x084C), "CLK_ENABLE_PHYCLK_HS1_CSIS0_RX_BYTE");
	PRINT_CLK((cmu_cam0 +  0x0850), "CLK_ENABLE_PHYCLK_HS2_CSIS0_RX_BYTE");
	PRINT_CLK((cmu_cam0 +  0x0854), "CLK_ENABLE_PHYCLK_HS3_CSIS0_RX_BYTE");
	PRINT_CLK((cmu_cam0 +  0x0858), "CLK_ENABLE_PHYCLK_HS0_CSIS1_RX_BYTE");
	PRINT_CLK((cmu_cam0 +  0x085C), "CLK_ENABLE_PHYCLK_HS1_CSIS1_RX_BYTE");

	printk(KERN_DEBUG "[@] CAM1\n");
	PRINT_CLK((cmu_cam1 + 0x0200), "CLK_CON_MUX_ACLK_CAM1_ARM_672_USER");
	PRINT_CLK((cmu_cam1 + 0x0204), "CLK_CON_MUX_ACLK_CAM1_TREX_VRA_528_USER");
	PRINT_CLK((cmu_cam1 + 0x0208), "CLK_CON_MUX_ACLK_CAM1_TREX_B_528_USER");
	PRINT_CLK((cmu_cam1 + 0x020C), "CLK_CON_MUX_ACLK_CAM1_BUS_264_USER");
	PRINT_CLK((cmu_cam1 + 0x0210), "CLK_CON_MUX_ACLK_CAM1_PERI_84_USER");
	PRINT_CLK((cmu_cam1 + 0x0214), "CLK_CON_MUX_ACLK_CAM1_CSIS2_414_USER");
	PRINT_CLK((cmu_cam1 + 0x0218), "CLK_CON_MUX_ACLK_CAM1_CSIS3_132_USER");
	PRINT_CLK((cmu_cam1 + 0x021C), "CLK_CON_MUX_ACLK_CAM1_SCL_566_USER");
	PRINT_CLK((cmu_cam1 + 0x0220), "CLK_CON_MUX_SCLK_CAM1_ISP_SPI0_USER");
	PRINT_CLK((cmu_cam1 + 0x0224), "CLK_CON_MUX_SCLK_CAM1_ISP_SPI1_USER");
	PRINT_CLK((cmu_cam1 + 0x0228), "CLK_CON_MUX_SCLK_CAM1_ISP_UART_USER");
	PRINT_CLK((cmu_cam1 + 0x022C), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS0_CSIS2_USER");
	PRINT_CLK((cmu_cam1 + 0x0230), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS1_CSIS2_USER");
	PRINT_CLK((cmu_cam1 + 0x0234), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS2_CSIS2_USER");
	PRINT_CLK((cmu_cam1 + 0x0238), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS3_CSIS2_USER");
	PRINT_CLK((cmu_cam1 + 0x023C), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS0_CSIS3_USER");
	PRINT_CLK((cmu_cam1 + 0x0400), "CLK_CON_DIV_PCLK_CAM1_ARM_168");
	PRINT_CLK((cmu_cam1 + 0x0408), "CLK_CON_DIV_PCLK_CAM1_TREX_VRA_264");
	PRINT_CLK((cmu_cam1 + 0x040C), "CLK_CON_DIV_PCLK_CAM1_BUS_132");
	PRINT_CLK((cmu_cam1 + 0x0800), "CLK_ENABLE_ACLK_CAM1_ARM_672");
	PRINT_CLK((cmu_cam1 + 0x0804), "CLK_ENABLE_PCLK_CAM1_ARM_168");
	PRINT_CLK((cmu_cam1 + 0x0808), "CLK_ENABLE_ACLK_CAM1_TREX_VRA_528");
	PRINT_CLK((cmu_cam1 + 0x080C), "CLK_ENABLE_PCLK_CAM1_TREX_VRA_264");
	PRINT_CLK((cmu_cam1 + 0x0810), "CLK_ENABLE_ACLK_CAM1_TREX_B_528");
	PRINT_CLK((cmu_cam1 + 0x0814), "CLK_ENABLE_ACLK_CAM1_BUS_264");
	PRINT_CLK((cmu_cam1 + 0x0818), "CLK_ENABLE_PCLK_CAM1_BUS_132");
	PRINT_CLK((cmu_cam1 + 0x081C), "CLK_ENABLE_PCLK_CAM1_PERI_84");
	PRINT_CLK((cmu_cam1 + 0x0820), "CLK_ENABLE_ACLK_CAM1_CSIS2_414");
	PRINT_CLK((cmu_cam1 + 0x0828), "CLK_ENABLE_ACLK_CAM1_CSIS3_132");
	PRINT_CLK((cmu_cam1 + 0x0830), "CLK_ENABLE_ACLK_CAM1_SCL_566");
	PRINT_CLK((cmu_cam1 + 0x083C), "CLK_ENABLE_PCLK_CAM1_SCL_283");
	PRINT_CLK((cmu_cam1 + 0x0840), "CLK_ENABLE_SCLK_CAM1_ISP_SPI0");
	PRINT_CLK((cmu_cam1 + 0x0844), "CLK_ENABLE_SCLK_CAM1_ISP_SPI1");
	PRINT_CLK((cmu_cam1 + 0x0848), "CLK_ENABLE_SCLK_CAM1_ISP_UART");
	PRINT_CLK((cmu_cam1 + 0x084C), "CLK_ENABLE_SCLK_ISP_PERI_IS_B");
	PRINT_CLK((cmu_cam1 + 0x0850), "CLK_ENABLE_PHYCLK_HS0_CSIS2_RX_BYTE");
	PRINT_CLK((cmu_cam1 + 0x0854), "CLK_ENABLE_PHYCLK_HS1_CSIS2_RX_BYTE");
	PRINT_CLK((cmu_cam1 + 0x0858), "CLK_ENABLE_PHYCLK_HS2_CSIS2_RX_BYTE");
	PRINT_CLK((cmu_cam1 + 0x085C), "CLK_ENABLE_PHYCLK_HS3_CSIS2_RX_BYTE");
	PRINT_CLK((cmu_cam1 + 0x0860), "CLK_ENABLE_PHYCLK_HS0_CSIS3_RX_BYTE");

	printk(KERN_DEBUG "[@] ISP0\n");
	PRINT_CLK((cmu_isp0 + 0x0200), "CLK_CON_MUX_ACLK_ISP0_528_USER");
	PRINT_CLK((cmu_isp0 + 0x0204), "CLK_CON_MUX_ACLK_ISP0_TPU_400_USER");
	PRINT_CLK((cmu_isp0 + 0x0208), "CLK_CON_MUX_ACLK_ISP0_TREX_528_USER");
	PRINT_CLK((cmu_isp0 + 0x0400), "CLK_CON_DIV_PCLK_ISP0");
	PRINT_CLK((cmu_isp0 + 0x0404), "CLK_CON_DIV_PCLK_ISP0_TPU");
	PRINT_CLK((cmu_isp0 + 0x0408), "CLK_CON_DIV_PCLK_ISP0_TREX_264");
	PRINT_CLK((cmu_isp0 + 0x040C), "CLK_CON_DIV_PCLK_ISP0_TREX_132");
	PRINT_CLK((cmu_isp0 + 0x0800), "CLK_ENABLE_ACLK_ISP0");
	PRINT_CLK((cmu_isp0 + 0x0808), "CLK_ENABLE_PCLK_ISP0");
	PRINT_CLK((cmu_isp0 + 0x080C), "CLK_ENABLE_ACLK_ISP0_TPU");
	PRINT_CLK((cmu_isp0 + 0x0814), "CLK_ENABLE_PCLK_ISP0_TPU");
	PRINT_CLK((cmu_isp0 + 0x0818), "CLK_ENABLE_ACLK_ISP0_TREX");
	PRINT_CLK((cmu_isp0 + 0x081C), "CLK_ENABLE_PCLK_TREX_264");
	PRINT_CLK((cmu_isp0 + 0x0824), "CLK_ENABLE_PCLK_TREX_132");
	PRINT_CLK((cmu_isp0 + 0x0820), "CLK_ENABLE_PCLK_HPM_APBIF_ISP0");
	PRINT_CLK((cmu_isp0 + 0x0828), "CLK_ENABLE_SCLK_PROMISE_ISP0");

	printk(KERN_DEBUG "[@] ISP1\n");
	PRINT_CLK((cmu_isp1 + 0x0200), "CLK_CON_MUX_ACLK_ISP1_468_USER");
	PRINT_CLK((cmu_isp1 + 0x0400), "CLK_CON_DIV_PCLK_ISP1_234");
	PRINT_CLK((cmu_isp1 + 0x0800), "CLK_ENABLE_ACLK_ISP1");
	PRINT_CLK((cmu_isp1 + 0x0808), "CLK_ENABLE_PCLK_ISP1_234");
	PRINT_CLK((cmu_isp1 + 0x0810), "CLK_ENABLE_SCLK_PROMISE_ISP1");
}

int exynos8890_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
	return 0;
}

int exynos8890_fimc_is_uart_gate(struct device *dev, bool mask)
{
	return 0;
}

int exynos8890_fimc_is_get_clk(struct device *dev)
{
	const char *name;
	struct clk *clk;
	u32 index;

	/* for debug */
	cmu_top = ioremap(0x10570000, SZ_4K);
	cmu_cam0 = ioremap(0x144D0000, SZ_4K);
	cmu_cam1 = ioremap(0x145D0000, SZ_4K);
	cmu_isp0 = ioremap(0x146D0000, SZ_4K);
	cmu_isp1 = ioremap(0x147D0000, SZ_4K);

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); ++index) {
		name = fimc_is_clk_list[index].name;
		if (!name) {
			pr_err("[@][ERR] %s: name is NULL\n", __func__);
			return -EINVAL;
		}

		clk = devm_clk_get(dev, name);
		if (IS_ERR_OR_NULL(clk)) {
			pr_err("[@][ERR] %s: could not lookup clock : %s\n", __func__, name);
			return -EINVAL;
		}

		fimc_is_clk_list[index].clk = clk;
	}

	return 0;
}

int exynos8890_fimc_is_cfg_clk(struct device *dev)
{
	pr_debug("%s\n", __func__);

	/* Clock Gating */
	exynos8890_fimc_is_uart_gate(dev, false);

	return 0;
}

int exynos8890_fimc_is_clk_on(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (pdata->clock_on) {
		ret = pdata->clk_off(dev);
		if (ret) {
			pr_err("clk_off is fail(%d)\n", ret);
			goto p_err;
		}
	}

	/* CAM0 */
	//fimc_is_enable(dev, "gate_csis0");
	//fimc_is_enable(dev, "gate_csis1");
	fimc_is_enable(dev, "gate_fimc_bns");
	fimc_is_enable(dev, "gate_fimc_3aa0");
	fimc_is_enable(dev, "gate_fimc_3aa1");
	fimc_is_enable(dev, "gate_hpm");
	//fimc_is_enable(dev, "pxmxdx_csis0");
	//fimc_is_enable(dev, "pxmxdx_csis1");
	fimc_is_enable(dev, "pxmxdx_csis2");
	fimc_is_enable(dev, "pxmxdx_csis3");
	fimc_is_enable(dev, "pxmxdx_3aa0");
	fimc_is_enable(dev, "pxmxdx_3aa1");
	//fimc_is_enable(dev, "pxmxdx_trex");
	//fimc_is_enable(dev, "hs0_csis0_rx_byte");
	//fimc_is_enable(dev, "hs1_csis0_rx_byte");
	//fimc_is_enable(dev, "hs2_csis0_rx_byte");
	//fimc_is_enable(dev, "hs3_csis0_rx_byte");
	//fimc_is_enable(dev, "hs0_csis1_rx_byte");
	//fimc_is_enable(dev, "hs1_csis1_rx_byte");

	/* CAM1 */
	fimc_is_enable(dev, "gate_isp_cpu");
	//fimc_is_enable(dev, "gate_csis2");
	//fimc_is_enable(dev, "gate_csis3");
	fimc_is_enable(dev, "gate_fimc_vra");
	fimc_is_enable(dev, "gate_mc_scaler");
	fimc_is_enable(dev, "gate_i2c0_isp");
	fimc_is_enable(dev, "gate_i2c1_isp");
	fimc_is_enable(dev, "gate_i2c2_isp");
	fimc_is_enable(dev, "gate_i2c3_isp");
	fimc_is_enable(dev, "gate_wdt_isp");
	fimc_is_enable(dev, "gate_mcuctl_isp");
	fimc_is_enable(dev, "gate_uart_isp");
	fimc_is_enable(dev, "gate_pdma_isp");
	fimc_is_enable(dev, "gate_pwm_isp");
	fimc_is_enable(dev, "gate_spi0_isp");
	fimc_is_enable(dev, "gate_spi1_isp");
	fimc_is_enable(dev, "isp_spi0");
	fimc_is_enable(dev, "isp_spi1");
	fimc_is_enable(dev, "isp_uart");
	fimc_is_enable(dev, "gate_sclk_pwm_isp");
	fimc_is_enable(dev, "gate_sclk_uart_isp");
	fimc_is_enable(dev, "cam1_arm");
	fimc_is_enable(dev, "cam1_vra");
	//fimc_is_enable(dev, "cam1_trex");
	//fimc_is_enable(dev, "cam1_bus");
	fimc_is_enable(dev, "cam1_peri");
	//fimc_is_enable(dev, "cam1_csis2");
	//fimc_is_enable(dev, "cam1_csis3");
	fimc_is_enable(dev, "cam1_scl");
	//fimc_is_enable(dev, "cam1_phy0_csis2");
	//fimc_is_enable(dev, "cam1_phy1_csis2");
	//fimc_is_enable(dev, "cam1_phy2_csis2");
	//fimc_is_enable(dev, "cam1_phy3_csis2");
	//fimc_is_enable(dev, "cam1_phy0_csis3");

	fimc_is_set_rate(dev, "isp_spi0", 100 * 1000000);
	fimc_is_set_rate(dev, "isp_spi1", 100 * 1000000);
	fimc_is_set_rate(dev, "isp_uart", 132 * 1000000);

	/* ISP0 */
	fimc_is_enable(dev, "gate_fimc_isp0");
	fimc_is_enable(dev, "gate_fimc_tpu");
	fimc_is_enable(dev, "isp0");
	fimc_is_enable(dev, "isp0_tpu");
	fimc_is_enable(dev, "isp0_trex");
	fimc_is_enable(dev, "pxmxdx_isp0_pxl");

	/* ISP1 */
	fimc_is_enable(dev, "gate_fimc_isp1");
	fimc_is_enable(dev, "isp1");

#ifdef DBG_DUMPCMU
	exynos8890_fimc_is_print_clk(dev);
#endif

	pdata->clock_on = true;

p_err:
	return 0;
}

int exynos8890_fimc_is_clk_off(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (!pdata->clock_on) {
		pr_err("clk_off is fail(already off)\n");
		ret = -EINVAL;
		goto p_err;
	}

	/* CAM0 */
	//fimc_is_disable(dev, "gate_csis0");
	//fimc_is_disable(dev, "gate_csis1");
	fimc_is_disable(dev, "gate_fimc_bns");
	fimc_is_disable(dev, "gate_fimc_3aa0");
	fimc_is_disable(dev, "gate_fimc_3aa1");
	fimc_is_disable(dev, "gate_hpm");
	//fimc_is_disable(dev, "pxmxdx_csis0");
	//fimc_is_disable(dev, "pxmxdx_csis1");
	fimc_is_disable(dev, "pxmxdx_csis2");
	fimc_is_disable(dev, "pxmxdx_csis3");
	fimc_is_disable(dev, "pxmxdx_3aa0");
	fimc_is_disable(dev, "pxmxdx_3aa1");
	//fimc_is_disable(dev, "pxmxdx_trex");
	//fimc_is_disable(dev, "hs0_csis0_rx_byte");
	//fimc_is_disable(dev, "hs1_csis0_rx_byte");
	//fimc_is_disable(dev, "hs2_csis0_rx_byte");
	//fimc_is_disable(dev, "hs3_csis0_rx_byte");
	//fimc_is_disable(dev, "hs0_csis1_rx_byte");
	//fimc_is_disable(dev, "hs1_csis1_rx_byte");

	/* CAM1 */
	fimc_is_disable(dev, "gate_isp_cpu");
	//fimc_is_disable(dev, "gate_csis2");
	//fimc_is_disable(dev, "gate_csis3");
	fimc_is_disable(dev, "gate_fimc_vra");
	fimc_is_disable(dev, "gate_mc_scaler");
	fimc_is_disable(dev, "gate_i2c0_isp");
	fimc_is_disable(dev, "gate_i2c1_isp");
	fimc_is_disable(dev, "gate_i2c2_isp");
	fimc_is_disable(dev, "gate_i2c3_isp");
	fimc_is_disable(dev, "gate_wdt_isp");
	fimc_is_disable(dev, "gate_mcuctl_isp");
	fimc_is_disable(dev, "gate_uart_isp");
	fimc_is_disable(dev, "gate_pdma_isp");
	fimc_is_disable(dev, "gate_pwm_isp");
	fimc_is_disable(dev, "gate_spi0_isp");
	fimc_is_disable(dev, "gate_spi1_isp");
	fimc_is_disable(dev, "isp_spi0");
	fimc_is_disable(dev, "isp_spi1");
	fimc_is_disable(dev, "isp_uart");
	fimc_is_disable(dev, "gate_sclk_pwm_isp");
	fimc_is_disable(dev, "gate_sclk_uart_isp");
	fimc_is_disable(dev, "cam1_arm");
	fimc_is_disable(dev, "cam1_vra");
	//fimc_is_disable(dev, "cam1_trex");
	//fimc_is_disable(dev, "cam1_bus");
	fimc_is_disable(dev, "cam1_peri");
	//fimc_is_disable(dev, "cam1_csis2");
	//fimc_is_disable(dev, "cam1_csis3");
	fimc_is_disable(dev, "cam1_scl");
	//fimc_is_disable(dev, "cam1_phy0_csis2");
	//fimc_is_disable(dev, "cam1_phy1_csis2");
	//fimc_is_disable(dev, "cam1_phy2_csis2");
	//fimc_is_disable(dev, "cam1_phy3_csis2");
	//fimc_is_disable(dev, "cam1_phy0_csis3");

	/* ISP0 */
	fimc_is_disable(dev, "gate_fimc_isp0");
	fimc_is_disable(dev, "gate_fimc_tpu");
	fimc_is_disable(dev, "isp0");
	fimc_is_disable(dev, "isp0_tpu");
	fimc_is_disable(dev, "isp0_trex");
	fimc_is_disable(dev, "pxmxdx_isp0_pxl");

	/* ISP1 */
	fimc_is_disable(dev, "gate_fimc_isp1");
	fimc_is_disable(dev, "isp1");

	pdata->clock_on = false;

p_err:
	return 0;
}

int exynos8890_fimc_is_print_clk(struct device *dev)
{
	printk(KERN_DEBUG "#################### ISP0 clock ####################\n");
	fimc_is_get_rate(dev, "isp0");
	fimc_is_get_rate(dev, "isp0_tpu");
	fimc_is_get_rate(dev, "isp0_trex");
	fimc_is_get_rate(dev, "pxmxdx_isp0_pxl");

	printk(KERN_DEBUG "#################### ISP1 clock ####################\n");
	fimc_is_get_rate(dev, "gate_fimc_isp1");
	fimc_is_get_rate(dev, "isp1");

	printk(KERN_DEBUG "#################### SENSOR clock ####################\n");
	fimc_is_get_rate(dev, "isp_sensor0");
	fimc_is_get_rate(dev, "isp_sensor1");
	fimc_is_get_rate(dev, "isp_sensor2");
	fimc_is_get_rate(dev, "isp_sensor3");

	printk(KERN_DEBUG "#################### CAM0 clock ####################\n");
	fimc_is_get_rate(dev, "gate_csis0");
	fimc_is_get_rate(dev, "gate_csis1");
	fimc_is_get_rate(dev, "gate_fimc_bns");
	fimc_is_get_rate(dev, "gate_fimc_3aa0");
	fimc_is_get_rate(dev, "gate_fimc_3aa1");
	fimc_is_get_rate(dev, "gate_hpm");
	fimc_is_get_rate(dev, "pxmxdx_csis0");
	fimc_is_get_rate(dev, "pxmxdx_csis1");
	fimc_is_get_rate(dev, "pxmxdx_csis2");
	fimc_is_get_rate(dev, "pxmxdx_csis3");
	fimc_is_get_rate(dev, "pxmxdx_3aa0");
	fimc_is_get_rate(dev, "pxmxdx_3aa1");
	fimc_is_get_rate(dev, "pxmxdx_trex");
	fimc_is_get_rate(dev, "hs0_csis0_rx_byte");
	fimc_is_get_rate(dev, "hs1_csis0_rx_byte");
	fimc_is_get_rate(dev, "hs2_csis0_rx_byte");
	fimc_is_get_rate(dev, "hs3_csis0_rx_byte");
	fimc_is_get_rate(dev, "hs0_csis1_rx_byte");
	fimc_is_get_rate(dev, "hs1_csis1_rx_byte");

	printk(KERN_DEBUG "#################### CAM1 clock ####################\n");
	fimc_is_get_rate(dev, "gate_isp_cpu");
	fimc_is_get_rate(dev, "gate_csis2");
	fimc_is_get_rate(dev, "gate_csis3");
	fimc_is_get_rate(dev, "gate_fimc_vra");
	fimc_is_get_rate(dev, "gate_mc_scaler");
	fimc_is_get_rate(dev, "gate_i2c0_isp");
	fimc_is_get_rate(dev, "gate_i2c1_isp");
	fimc_is_get_rate(dev, "gate_i2c2_isp");
	fimc_is_get_rate(dev, "gate_i2c3_isp");
	fimc_is_get_rate(dev, "gate_wdt_isp");
	fimc_is_get_rate(dev, "gate_mcuctl_isp");
	fimc_is_get_rate(dev, "gate_uart_isp");
	fimc_is_get_rate(dev, "gate_pdma_isp");
	fimc_is_get_rate(dev, "gate_pwm_isp");
	fimc_is_get_rate(dev, "gate_spi0_isp");
	fimc_is_get_rate(dev, "gate_spi1_isp");
	fimc_is_get_rate(dev, "isp_spi0");
	fimc_is_get_rate(dev, "isp_spi1");
	fimc_is_get_rate(dev, "isp_uart");
	fimc_is_get_rate(dev, "gate_sclk_pwm_isp");
	fimc_is_get_rate(dev, "gate_sclk_uart_isp");
	fimc_is_get_rate(dev, "cam1_arm");
	fimc_is_get_rate(dev, "cam1_vra");
	fimc_is_get_rate(dev, "cam1_trex");
	fimc_is_get_rate(dev, "cam1_bus");
	fimc_is_get_rate(dev, "cam1_peri");
	fimc_is_get_rate(dev, "cam1_csis2");
	fimc_is_get_rate(dev, "cam1_csis3");
	fimc_is_get_rate(dev, "cam1_scl");
	fimc_is_get_rate(dev, "cam1_phy0_csis2");
	fimc_is_get_rate(dev, "cam1_phy1_csis2");
	fimc_is_get_rate(dev, "cam1_phy2_csis2");
	fimc_is_get_rate(dev, "cam1_phy3_csis2");
	fimc_is_get_rate(dev, "cam1_phy0_csis3");

	exynos8890_fimc_is_print_clk_reg();

	return 0;
}

#elif defined(CONFIG_SOC_EXYNOS7570)
void __iomem *cmu_mif;
void __iomem *cmu_isp;
void __iomem *cmu_mcsl;

int exynos7570_fimc_is_print_clk(struct device *dev);
static void exynos7570_fimc_is_print_clk_reg(void)
{
	printk(KERN_DEBUG "[@] MIF\n");
	PRINT_CLK((cmu_mif + 0x0600), "CLK_STAT_MUX_SHARED0_PLL");
	PRINT_CLK((cmu_mif + 0x0604), "CLK_STAT_MUX_SHARED1_PLL");
	PRINT_CLK((cmu_mif + 0x0608), "CLK_STAT_MUX_SHARED2_PLL");
	PRINT_CLK((cmu_mif + 0x022C), "CLK_CON_MUX_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_mif + 0x0230), "CLK_CON_MUX_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_mif + 0x0244), "CLK_CON_MUX_CLKCMU_MFCMSCL_MFC");
	PRINT_CLK((cmu_mif + 0x0280), "CLK_CON_MUX_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_mif + 0x0428), "CLK_CON_DIV_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_mif + 0x042C), "CLK_CON_DIV_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_mif + 0x0440), "CLK_CON_DIV_CLKCMU_MFCMSCL_MFC");
	PRINT_CLK((cmu_mif + 0x04C4), "CLK_CON_DIV_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_mif + 0x062C), "CLK_STAT_MUX_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_mif + 0x0630), "CLK_STAT_MUX_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_mif + 0x0644), "CLK_STAT_MUX_CLKCMU_MFCMSCL_MFC");
	PRINT_CLK((cmu_mif + 0x0680), "CLK_STAT_MUX_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_mif + 0x0834), "CLK_ENABLE_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_mif + 0x0838), "CLK_ENABLE_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_mif + 0x0848), "CLK_ENABLE_CLKCMU_MFCMSCL_MFC");
	PRINT_CLK((cmu_mif + 0x0888), "CLK_ENABLE_CLKCMU_ISP_SENSOR0");

	printk(KERN_DEBUG "[@] MCSL\n");
	PRINT_CLK((cmu_mcsl + 0x0204), "CLK_CON_MUX_CLKCMU_MFCMSCL_MFC_USER");
	PRINT_CLK((cmu_mcsl + 0x0400), "CLK_CON_DIV_CLK_MFCMSCL_APB");
	PRINT_CLK((cmu_mcsl + 0x0604), "CLK_STAT_MUX_CLKCMU_MFCMSCL_MFC_USER");
	PRINT_CLK((cmu_mcsl + 0x0808), "CLK_ENABLE_CLK_MFCMSCL_MFCBUS_PLL_CON0");

	printk(KERN_DEBUG "[@] ISP\n");
	PRINT_CLK((cmu_isp + 0x0210), "CLK_CON_MUX_CLKCMU_ISP_VRA_USER");
	PRINT_CLK((cmu_isp + 0x0214), "CLK_CON_MUX_CLKCMU_ISP_CAM_USER");
	PRINT_CLK((cmu_isp + 0x0230), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS0_S4_USER");
	PRINT_CLK((cmu_isp + 0x0404), "CLK_CON_DIV_CLK_ISP_CAM_HALF");
	PRINT_CLK((cmu_isp + 0x0610), "CLK_STAT_MUX_CLKCMU_ISP_VRA_USER");
	PRINT_CLK((cmu_isp + 0x0614), "CLK_STAT_MUX_CLKCMU_ISP_CAM_USER");
	PRINT_CLK((cmu_isp + 0x0630), "CLK_STAT_MUX_CLKPHY_ISP_S_RXBYTECLKHS0_S4_USER");
	PRINT_CLK((cmu_isp + 0x0810), "CLK_ENABLE_CLK_ISP_VRA");
	PRINT_CLK((cmu_isp + 0x081C), "CLK_ENABLE_CLK_ISP_CAM");
	PRINT_CLK((cmu_isp + 0x0828), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS0_S4");
}

int exynos7570_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
	return 0;
}

int exynos7570_fimc_is_uart_gate(struct device *dev, bool mask)
{
	return 0;
}

int exynos7570_fimc_is_get_clk(struct device *dev)
{
	const char *name;
	struct clk *clk;
	u32 index;

	/* for debug */
	cmu_mif = ioremap(0x10460000, SZ_4K);
	cmu_isp = ioremap(0x144D0000, SZ_4K);
	cmu_mcsl = ioremap(0x12CB0000, SZ_4K);

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); ++index) {
		name = fimc_is_clk_list[index].name;
		if (!name) {
			pr_err("[@][ERR] %s: name is NULL\n", __func__);
			return -EINVAL;
		}

		clk = devm_clk_get(dev, name);
		if (IS_ERR_OR_NULL(clk)) {
			pr_err("[@][ERR] %s: could not lookup clock : %s\n", __func__, name);
			return -EINVAL;
		}

		fimc_is_clk_list[index].clk = clk;
	}

	return 0;
}

int exynos7570_fimc_is_cfg_clk(struct device *dev)
{
	pr_debug("%s\n", __func__);

	/* Clock Gating */
	exynos7570_fimc_is_uart_gate(dev, false);

	return 0;
}

int exynos7570_fimc_is_clk_on(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (pdata->clock_on) {
		ret = pdata->clk_off(dev);
		if (ret) {
			pr_err("clk_off is fail(%d)\n", ret);
			goto p_err;
		}
	}

	/* ISP */
	fimc_is_enable(dev, "isp_ppmu");
	fimc_is_enable(dev, "isp_bts");
	fimc_is_enable(dev, "isp_cam");
	fimc_is_enable(dev, "isp_vra");
	fimc_is_enable(dev, "gate_mscl");

#ifdef DBG_DUMPCMU
	exynos7570_fimc_is_print_clk(dev);
#endif

	pdata->clock_on = true;

p_err:
	return 0;
}

int exynos7570_fimc_is_clk_off(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (!pdata->clock_on) {
		pr_err("clk_off is fail(already off)\n");
		ret = -EINVAL;
		goto p_err;
	}

	/* ISP */
	fimc_is_disable(dev, "isp_ppmu");
	fimc_is_disable(dev, "isp_bts");
	fimc_is_disable(dev, "isp_cam");
	fimc_is_disable(dev, "isp_vra");
	fimc_is_disable(dev, "gate_mscl");

	pdata->clock_on = false;

p_err:
	return 0;
}

int exynos7570_fimc_is_print_clk(struct device *dev)
{
	printk(KERN_DEBUG "#################### SENSOR clock ####################\n");
	fimc_is_get_rate(dev, "mif_isp_sensor0");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");

	printk(KERN_DEBUG "#################### ISP clock ####################\n");
	fimc_is_get_rate(dev, "isp_ppmu");
	fimc_is_get_rate(dev, "isp_bts");
	fimc_is_get_rate(dev, "isp_cam");
	fimc_is_get_rate(dev, "isp_vra");
	fimc_is_get_rate(dev, "gate_mscl");

	exynos7570_fimc_is_print_clk_reg();

	return 0;
}

#elif defined(CONFIG_SOC_EXYNOS7870)
/* for debug */
void __iomem *cmu_mif;
void __iomem *cmu_isp;

int exynos7870_fimc_is_print_clk(struct device *dev);
static void exynos7870_fimc_is_print_clk_reg(void)
{
	printk(KERN_DEBUG "[@] MIF\n");
	PRINT_CLK((cmu_mif + 0x0140), "BUS_PLL_CON0");
	PRINT_CLK((cmu_mif + 0x0144), "BUS_PLL_CON1");
	PRINT_CLK((cmu_mif + 0x0208), "CLK_CON_MUX_BUS_PLL");
	PRINT_CLK((cmu_mif + 0x0264), "CLK_CON_MUX_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_mif + 0x0268), "CLK_CON_MUX_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_mif + 0x026C), "CLK_CON_MUX_CLKCMU_ISP_ISP");
	PRINT_CLK((cmu_mif + 0x02C4), "CLK_CON_MUX_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_mif + 0x02C8), "CLK_CON_MUX_CLKCMU_ISP_SENSOR1");
	PRINT_CLK((cmu_mif + 0x02CC), "CLK_CON_MUX_CLKCMU_ISP_SENSOR2");
	PRINT_CLK((cmu_mif + 0x0464), "CLK_CON_DIV_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_mif + 0x0468), "CLK_CON_DIV_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_mif + 0x046C), "CLK_CON_DIV_CLKCMU_ISP_ISP");
	PRINT_CLK((cmu_mif + 0x04C4), "CLK_CON_DIV_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_mif + 0x04C8), "CLK_CON_DIV_CLKCMU_ISP_SENSOR1");
	PRINT_CLK((cmu_mif + 0x04CC), "CLK_CON_DIV_CLKCMU_ISP_SENSOR2");
	PRINT_CLK((cmu_mif + 0x0864), "CLK_ENABLE_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_mif + 0x0868), "CLK_ENABLE_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_mif + 0x086C), "CLK_ENABLE_CLKCMU_ISP_ISP");
	PRINT_CLK((cmu_mif + 0x08C4), "CLK_ENABLE_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_mif + 0x08C8), "CLK_ENABLE_CLKCMU_ISP_SENSOR1");
	PRINT_CLK((cmu_mif + 0x08CC), "CLK_ENABLE_CLKCMU_ISP_SENSOR2");

	printk(KERN_DEBUG "[@] ISP\n");
	PRINT_CLK((cmu_isp +  0x0100), "ISP_PLL_CON0");
	PRINT_CLK((cmu_isp +  0x0104), "ISP_PLL_CON1");
	PRINT_CLK((cmu_isp +  0x0200), "CLK_CON_MUX_ISP_PLL");
	PRINT_CLK((cmu_isp +  0x0210), "CLK_CON_MUX_CLKCMU_ISP_VRA_USER");
	PRINT_CLK((cmu_isp +  0x0214), "CLK_CON_MUX_CLKCMU_ISP_CAM_USER");
	PRINT_CLK((cmu_isp +  0x0218), "CLK_CON_MUX_CLKCMU_ISP_ISP_USER");
	PRINT_CLK((cmu_isp +  0x0220), "CLK_CON_MUX_CLK_ISP_VRA");
	PRINT_CLK((cmu_isp +  0x0224), "CLK_CON_MUX_CLK_ISP_CAM");
	PRINT_CLK((cmu_isp +  0x0228), "CLK_CON_MUX_CLK_ISP_ISP");
	PRINT_CLK((cmu_isp +  0x022C), "CLK_CON_MUX_CLK_ISP_ISPD");
	PRINT_CLK((cmu_isp +  0x0230), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS0_S4_USER");
	PRINT_CLK((cmu_isp +  0x0234), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS0_S4S_USER");
	PRINT_CLK((cmu_isp +  0x0400), "CLK_CON_DIV_CLK_ISP_APB");
	PRINT_CLK((cmu_isp +  0x0404), "CLK_CON_DIV_CLK_CAM_HALF");
	PRINT_CLK((cmu_isp +  0x0810), "CLK_ENABLE_CLK_ISP_VRA");
	PRINT_CLK((cmu_isp +  0x0814), "CLK_ENABLE_CLK_ISP_APB");
	PRINT_CLK((cmu_isp +  0x0818), "CLK_ENABLE_CLK_ISP_ISPD");
	PRINT_CLK((cmu_isp +  0x081C), "CLK_ENABLE_CLK_ISP_CAM");
	PRINT_CLK((cmu_isp +  0x0820), "CLK_ENABLE_CLK_ISP_CAM_HALF");
	PRINT_CLK((cmu_isp +  0x0824), "CLK_ENABLE_CLK_ISP_ISP");
	PRINT_CLK((cmu_isp +  0x0828), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS0_S4");
	PRINT_CLK((cmu_isp +  0x082C), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS0_S4S");
}

int exynos7870_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
	return 0;
}

int exynos7870_fimc_is_uart_gate(struct device *dev, bool mask)
{
	return 0;
}

int exynos7870_fimc_is_get_clk(struct device *dev)
{
	const char *name;
	struct clk *clk;
	u32 index;

	/* for debug */
	cmu_mif = ioremap(0x10460000, SZ_4K);
	cmu_isp = ioremap(0x144D0000, SZ_4K);

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); ++index) {
		name = fimc_is_clk_list[index].name;
		if (!name) {
			pr_err("[@][ERR] %s: name is NULL\n", __func__);
			return -EINVAL;
		}

		clk = devm_clk_get(dev, name);
		if (IS_ERR_OR_NULL(clk)) {
			pr_err("[@][ERR] %s: could not lookup clock : %s\n", __func__, name);
			return -EINVAL;
		}

		fimc_is_clk_list[index].clk = clk;
	}

	return 0;
}

int exynos7870_fimc_is_cfg_clk(struct device *dev)
{
	pr_debug("%s\n", __func__);

	/* Clock Gating */
	exynos7870_fimc_is_uart_gate(dev, false);

	return 0;
}

int exynos7870_fimc_is_clk_on(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (pdata->clock_on) {
		ret = pdata->clk_off(dev);
		if (ret) {
			pr_err("clk_off is fail(%d)\n", ret);
			goto p_err;
		}
	}

	/* ISP */
	fimc_is_enable(dev, "gate_isp_sysmmu");
	fimc_is_enable(dev, "gate_isp_ppmu");
	fimc_is_enable(dev, "gate_isp_bts");
	fimc_is_enable(dev, "gate_isp_cam");
	fimc_is_enable(dev, "gate_isp_isp");
	fimc_is_enable(dev, "gate_isp_vra");
	fimc_is_enable(dev, "pxmxdx_isp_isp");
	fimc_is_enable(dev, "pxmxdx_isp_cam");
	fimc_is_enable(dev, "pxmxdx_isp_vra");

#ifdef DBG_DUMPCMU
	exynos7870_fimc_is_print_clk(dev);
#endif

	pdata->clock_on = true;

p_err:
	return 0;
}

int exynos7870_fimc_is_clk_off(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (!pdata->clock_on) {
		pr_err("clk_off is fail(already off)\n");
		ret = -EINVAL;
		goto p_err;
	}

	/* ISP */
	fimc_is_disable(dev, "gate_isp_sysmmu");
	fimc_is_disable(dev, "gate_isp_ppmu");
	fimc_is_disable(dev, "gate_isp_bts");
	fimc_is_disable(dev, "gate_isp_cam");
	fimc_is_disable(dev, "gate_isp_isp");
	fimc_is_disable(dev, "gate_isp_vra");
	fimc_is_disable(dev, "pxmxdx_isp_isp");
	fimc_is_disable(dev, "pxmxdx_isp_cam");
	fimc_is_disable(dev, "pxmxdx_isp_vra");

	pdata->clock_on = false;

p_err:
	return 0;
}

int exynos7870_fimc_is_print_clk(struct device *dev)
{
	printk(KERN_DEBUG "#################### SENSOR clock ####################\n");
	fimc_is_get_rate(dev, "isp_sensor0_sclk");
	fimc_is_get_rate(dev, "isp_sensor1_sclk");
	fimc_is_get_rate(dev, "isp_sensor2_sclk");

	printk(KERN_DEBUG "#################### ISP clock ####################\n");
	fimc_is_get_rate(dev, "gate_isp_sysmmu");
	fimc_is_get_rate(dev, "gate_isp_ppmu");
	fimc_is_get_rate(dev, "gate_isp_bts");
	fimc_is_get_rate(dev, "gate_isp_cam");
	fimc_is_get_rate(dev, "gate_isp_isp");
	fimc_is_get_rate(dev, "gate_isp_vra");
	fimc_is_get_rate(dev, "pxmxdx_isp_isp");
	fimc_is_get_rate(dev, "pxmxdx_isp_cam");
	fimc_is_get_rate(dev, "pxmxdx_isp_vra");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user");

	exynos7870_fimc_is_print_clk_reg();

	return 0;
}

#elif defined(CONFIG_SOC_EXYNOS7880)
/* for debug */
void __iomem *cmu_ccore;
void __iomem *cmu_isp;

int exynos7880_fimc_is_print_clk(struct device *dev);
static void exynos7880_fimc_is_print_clk_reg(void)
{
	printk(KERN_DEBUG "[@] CCORE\n");
	PRINT_CLK((cmu_ccore + 0x0120), "MEDIA_PLL_CON0");
	PRINT_CLK((cmu_ccore + 0x0124), "MEDIA_PLL_CON1");
	PRINT_CLK((cmu_ccore + 0x0140), "BUS_PLL_CON0");
	PRINT_CLK((cmu_ccore + 0x0144), "BUS_PLL_CON1");
	PRINT_CLK((cmu_ccore + 0x0220), "CLK_CON_MUX_MEDIA_PLL");
	PRINT_CLK((cmu_ccore + 0x0224), "CLK_CON_MUX_BUS_PLL");
	PRINT_CLK((cmu_ccore + 0x0264), "CLK_CON_MUX_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_ccore + 0x0268), "CLK_CON_MUX_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_ccore + 0x026C), "CLK_CON_MUX_CLKCMU_ISP_ISP");
	PRINT_CLK((cmu_ccore + 0x02C4), "CLK_CON_MUX_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_ccore + 0x02C8), "CLK_CON_MUX_CLKCMU_ISP_SENSOR1");
	PRINT_CLK((cmu_ccore + 0x02CC), "CLK_CON_MUX_CLKCMU_ISP_SENSOR2");
	PRINT_CLK((cmu_ccore + 0x0464), "CLK_CON_DIV_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_ccore + 0x0468), "CLK_CON_DIV_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_ccore + 0x046C), "CLK_CON_DIV_CLKCMU_ISP_ISP");
	PRINT_CLK((cmu_ccore + 0x04C4), "CLK_CON_DIV_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_ccore + 0x04C8), "CLK_CON_DIV_CLKCMU_ISP_SENSOR1");
	PRINT_CLK((cmu_ccore + 0x04CC), "CLK_CON_DIV_CLKCMU_ISP_SENSOR2");
	PRINT_CLK((cmu_ccore + 0x0864), "CLK_ENABLE_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_ccore + 0x0868), "CLK_ENABLE_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_ccore + 0x086C), "CLK_ENABLE_CLKCMU_ISP_ISP");
	PRINT_CLK((cmu_ccore + 0x08C4), "CLK_ENABLE_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_ccore + 0x08C8), "CLK_ENABLE_CLKCMU_ISP_SENSOR1");
	PRINT_CLK((cmu_ccore + 0x08CC), "CLK_ENABLE_CLKCMU_ISP_SENSOR2");

	printk(KERN_DEBUG "[@] ISP\n");
	PRINT_CLK((cmu_isp +  0x0100), "ISP_PLL_CON0");
	PRINT_CLK((cmu_isp +  0x0104), "ISP_PLL_CON1");
	PRINT_CLK((cmu_isp +  0x0200), "CLK_CON_MUX_ISP_PLL");
	PRINT_CLK((cmu_isp +  0x0210), "CLK_CON_MUX_CLKCMU_ISP_VRA_USER");
	PRINT_CLK((cmu_isp +  0x0214), "CLK_CON_MUX_CLKCMU_ISP_CAM_USER");
	PRINT_CLK((cmu_isp +  0x0218), "CLK_CON_MUX_CLKCMU_ISP_ISP_USER");
	PRINT_CLK((cmu_isp +  0x0220), "CLK_CON_MUX_CLK_ISP_VRA");
	PRINT_CLK((cmu_isp +  0x0224), "CLK_CON_MUX_CLK_ISP_CAM");
	PRINT_CLK((cmu_isp +  0x0228), "CLK_CON_MUX_CLK_ISP_ISP");
	PRINT_CLK((cmu_isp +  0x022C), "CLK_CON_MUX_CLK_ISP_ISPD");
	PRINT_CLK((cmu_isp +  0x0230), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS0_S4_USER");
	PRINT_CLK((cmu_isp +  0x0234), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS1_S4_USER");
	PRINT_CLK((cmu_isp +  0x0238), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS2_S4_USER");
	PRINT_CLK((cmu_isp +  0x023C), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS3_S4_USER");
	PRINT_CLK((cmu_isp +  0x0240), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS0_S4S_USER");
	PRINT_CLK((cmu_isp +  0x0244), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS1_S4S_USER");
	PRINT_CLK((cmu_isp +  0x0248), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS2_S4S_USER");
	PRINT_CLK((cmu_isp +  0x024C), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS3_S4S_USER");
	PRINT_CLK((cmu_isp +  0x0400), "CLK_CON_DIV_CLK_ISP_APB");
	PRINT_CLK((cmu_isp +  0x0404), "CLK_CON_DIV_CLK_CAM_HALF");
	PRINT_CLK((cmu_isp +  0x0810), "CLK_ENABLE_CLK_ISP_VRA");
	PRINT_CLK((cmu_isp +  0x0814), "CLK_ENABLE_CLK_ISP_APB");
	PRINT_CLK((cmu_isp +  0x0818), "CLK_ENABLE_CLK_ISP_ISPD");
	PRINT_CLK((cmu_isp +  0x081C), "CLK_ENABLE_CLK_ISP_CAM");
	PRINT_CLK((cmu_isp +  0x0820), "CLK_ENABLE_CLK_ISP_CAM_HALF");
	PRINT_CLK((cmu_isp +  0x0824), "CLK_ENABLE_CLK_ISP_ISP");
	PRINT_CLK((cmu_isp +  0x0828), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS0_S4");
	PRINT_CLK((cmu_isp +  0x082C), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS1_S4");
	PRINT_CLK((cmu_isp +  0x0830), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS2_S4");
	PRINT_CLK((cmu_isp +  0x0834), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS3_S4");
	PRINT_CLK((cmu_isp +  0x0838), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS0_S4S");
	PRINT_CLK((cmu_isp +  0x083C), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS1_S4S");
	PRINT_CLK((cmu_isp +  0x0840), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS2_S4S");
	PRINT_CLK((cmu_isp +  0x084C), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS3_S4S");
}

int exynos7880_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
	return 0;
}

int exynos7880_fimc_is_uart_gate(struct device *dev, bool mask)
{
	return 0;
}

int exynos7880_fimc_is_get_clk(struct device *dev)
{
	const char *name;
	struct clk *clk;
	size_t index;

	/* for debug */
	cmu_ccore = ioremap(0x10680000, SZ_4K);
	cmu_isp = ioremap(0x144D0000, SZ_4K);

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); ++index) {
		name = fimc_is_clk_list[index].name;
		if (!name) {
			pr_err("[@][ERR] %s: name is NULL\n", __func__);
			return -EINVAL;
		}

		clk = devm_clk_get(dev, name);
		if (IS_ERR_OR_NULL(clk)) {
			pr_err("[@][ERR] %s: could not lookup clock : %s\n", __func__, name);
			return -EINVAL;
		}

		fimc_is_clk_list[index].clk = clk;
	}

	return 0;
}

int exynos7880_fimc_is_cfg_clk(struct device *dev)
{
	pr_debug("%s\n", __func__);

	/* Clock Gating */
	exynos7880_fimc_is_uart_gate(dev, false);

	return 0;
}

int exynos7880_fimc_is_clk_on(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (pdata->clock_on) {
		ret = pdata->clk_off(dev);
		if (ret) {
			pr_err("clk_off is fail(%d)\n", ret);
			goto p_err;
		}
	}

#ifdef DBG_DUMPCMU
	exynos7880_fimc_is_print_clk(dev);
#endif

	/* ISP */
	fimc_is_enable(dev, "gate_isp_sysmmu");
	fimc_is_enable(dev, "gate_isp_ppmu");
	fimc_is_enable(dev, "gate_isp_bts");
	fimc_is_enable(dev, "gate_isp_cam");
	fimc_is_enable(dev, "gate_isp_isp");
	fimc_is_enable(dev, "gate_isp_vra");
	fimc_is_enable(dev, "pxmxdx_isp_isp");
	fimc_is_enable(dev, "pxmxdx_isp_cam");
	fimc_is_enable(dev, "pxmxdx_isp_vra");

	pdata->clock_on = true;

p_err:
	return 0;
}

int exynos7880_fimc_is_clk_off(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (!pdata->clock_on) {
		pr_err("clk_off is fail(already off)\n");
		ret = -EINVAL;
		goto p_err;
	}

	/* ISP */
	fimc_is_disable(dev, "gate_isp_sysmmu");
	fimc_is_disable(dev, "gate_isp_ppmu");
	fimc_is_disable(dev, "gate_isp_bts");
	fimc_is_disable(dev, "gate_isp_cam");
	fimc_is_disable(dev, "gate_isp_isp");
	fimc_is_disable(dev, "gate_isp_vra");
	fimc_is_disable(dev, "pxmxdx_isp_isp");
	fimc_is_disable(dev, "pxmxdx_isp_cam");
	fimc_is_disable(dev, "pxmxdx_isp_vra");

	pdata->clock_on = false;

p_err:
	return 0;
}

int exynos7880_fimc_is_print_clk(struct device *dev)
{
	printk(KERN_DEBUG "#################### SENSOR clock ####################\n");
	fimc_is_get_rate(dev, "isp_sensor0_sclk");
	fimc_is_get_rate(dev, "isp_sensor1_sclk");
	fimc_is_get_rate(dev, "isp_sensor2_sclk");

	printk(KERN_DEBUG "#################### ISP clock ####################\n");
	fimc_is_get_rate(dev, "gate_isp_sysmmu");
	fimc_is_get_rate(dev, "gate_isp_ppmu");
	fimc_is_get_rate(dev, "gate_isp_bts");
	fimc_is_get_rate(dev, "gate_isp_cam");
	fimc_is_get_rate(dev, "gate_isp_isp");
	fimc_is_get_rate(dev, "gate_isp_vra");
	fimc_is_get_rate(dev, "pxmxdx_isp_isp");
	fimc_is_get_rate(dev, "pxmxdx_isp_cam");
	fimc_is_get_rate(dev, "pxmxdx_isp_vra");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs1_s4_user");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs2_s4_user");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs3_s4_user");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs1_s4s_user");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs2_s4s_user");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs3_s4s_user");

	exynos7880_fimc_is_print_clk_reg();

	return 0;
}
#endif

/* Wrapper functions */
int exynos_fimc_is_clk_get(struct device *dev)
{
#if defined(CONFIG_SOC_EXYNOS8895)
       exynos8895_fimc_is_get_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS8890)
       exynos8890_fimc_is_get_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS7570)
       exynos7570_fimc_is_get_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS7870)
       exynos7870_fimc_is_get_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS7880)
       exynos7880_fimc_is_get_clk(dev);
#else
#error exynos_fimc_is_clk_get is not implemented
#endif
       return 0;
}

int exynos_fimc_is_clk_cfg(struct device *dev)
{
#if defined(CONFIG_SOC_EXYNOS8895)
	exynos8895_fimc_is_cfg_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS8890)
	exynos8890_fimc_is_cfg_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS7570)
	exynos7570_fimc_is_cfg_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS7870)
	exynos7870_fimc_is_cfg_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS7880)
	exynos7880_fimc_is_cfg_clk(dev);
#else
#error exynos_fimc_is_clk_cfg is not implemented
#endif
	return 0;
}

int exynos_fimc_is_clk_on(struct device *dev)
{
#if defined(CONFIG_SOC_EXYNOS8895)
	exynos8895_fimc_is_clk_on(dev);
#elif defined(CONFIG_SOC_EXYNOS8890)
	exynos8890_fimc_is_clk_on(dev);
#elif defined(CONFIG_SOC_EXYNOS7570)
	exynos7570_fimc_is_clk_on(dev);
#elif defined(CONFIG_SOC_EXYNOS7870)
	exynos7870_fimc_is_clk_on(dev);
#elif defined(CONFIG_SOC_EXYNOS7880)
	exynos7880_fimc_is_clk_on(dev);
#else
#error exynos_fimc_is_clk_on is not implemented
#endif
	return 0;
}

int exynos_fimc_is_clk_off(struct device *dev)
{
#if defined(CONFIG_SOC_EXYNOS8895)
	exynos8895_fimc_is_clk_off(dev);
#elif defined(CONFIG_SOC_EXYNOS8890)
	exynos8890_fimc_is_clk_off(dev);
#elif defined(CONFIG_SOC_EXYNOS7570)
	exynos7570_fimc_is_clk_off(dev);
#elif defined(CONFIG_SOC_EXYNOS7870)
	exynos7870_fimc_is_clk_off(dev);
#elif defined(CONFIG_SOC_EXYNOS7880)
	exynos7880_fimc_is_clk_off(dev);
#else
#error exynos_fimc_is_clk_off is not implemented
#endif
	return 0;
}

int exynos_fimc_is_print_clk(struct device *dev)
{
#if defined(CONFIG_SOC_EXYNOS8895)
	exynos8895_fimc_is_print_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS8890)
	exynos8890_fimc_is_print_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS7570)
	exynos7570_fimc_is_print_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS7870)
	exynos7870_fimc_is_print_clk(dev);
#elif defined(CONFIG_SOC_EXYNOS7880)
	exynos7880_fimc_is_print_clk(dev);
#else
#error exynos_fimc_is_print_clk is not implemented
#endif
	return 0;
}

int exynos_fimc_is_set_user_clk_gate(u32 group_id, bool is_on,
	u32 user_scenario_id,
	unsigned long msk_state,
	struct exynos_fimc_is_clk_gate_info *gate_info)
{
	return 0;
}

int exynos_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
#if defined(CONFIG_SOC_EXYNOS8895)
	exynos8895_fimc_is_clk_gate(clk_gate_id, is_on);
#elif defined(CONFIG_SOC_EXYNOS8890)
	exynos8890_fimc_is_clk_gate(clk_gate_id, is_on);
#elif defined(CONFIG_SOC_EXYNOS7570)
	exynos7570_fimc_is_clk_gate(clk_gate_id, is_on);
#elif defined(CONFIG_SOC_EXYNOS7870)
	exynos7870_fimc_is_clk_gate(clk_gate_id, is_on);
#elif defined(CONFIG_SOC_EXYNOS7880)
	exynos7880_fimc_is_clk_gate(clk_gate_id, is_on);
#endif
	return 0;
}
