/* linux/arch/arm/mach-exynos/setup-fimc-sensor.c
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
#include <exynos-fimc-is-sensor.h>
#include <exynos-fimc-is-module.h>

#if defined(CONFIG_SOC_EXYNOS8895)
static int exynos8895_fimc_is_csi_gate(struct device *dev, u32 instance, bool mask)
{
	int ret = 0;

	pr_debug("%s(instance : %d / mask : %d)\n", __func__, instance, mask);

	switch (instance) {
	case 0:
		if (mask) {
			fimc_is_disable(dev, "MUX_CIS_CLK0");
			fimc_is_disable(dev, "GATE_IS_CAM_CSIS0");
		} else {
			fimc_is_enable(dev, "MUX_CIS_CLK0");
			fimc_is_enable(dev, "GATE_IS_CAM_CSIS0");
		}
		break;
	case 1:
		if (mask) {
			fimc_is_disable(dev, "MUX_CIS_CLK1");
			fimc_is_disable(dev, "GATE_IS_CAM_CSIS1");
		} else {
			fimc_is_enable(dev, "MUX_CIS_CLK1");
			fimc_is_enable(dev, "GATE_IS_CAM_CSIS1");
		}
		break;
	case 2:
		if (mask) {
			fimc_is_disable(dev, "MUX_CIS_CLK2");
			fimc_is_disable(dev, "GATE_IS_CAM_CSIS2");
		} else {
			fimc_is_enable(dev, "MUX_CIS_CLK2");
			fimc_is_enable(dev, "GATE_IS_CAM_CSIS2");
		}
		break;
	case 3:
		if (mask) {
			fimc_is_disable(dev, "MUX_CIS_CLK3");
			fimc_is_disable(dev, "GATE_IS_CAM_CSIS3");
		} else {
			fimc_is_enable(dev, "MUX_CIS_CLK3");
			fimc_is_enable(dev, "GATE_IS_CAM_CSIS3");
		}
		break;
	default:
		pr_err("(%s) instance is invalid(%d)\n", __func__, instance);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int exynos8895_fimc_is_sensor_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
	fimc_is_enable(dev, "MUX_CIS_CLK0");
	fimc_is_enable(dev, "MUX_CIS_CLK1");
	fimc_is_enable(dev, "MUX_CIS_CLK2");
	fimc_is_enable(dev, "MUX_CIS_CLK3");

	fimc_is_enable(dev, "GATE_IS_CAM_CSIS0");
	fimc_is_enable(dev, "GATE_IS_CAM_CSIS1");
	fimc_is_enable(dev, "GATE_IS_CAM_CSIS2");
	fimc_is_enable(dev, "GATE_IS_CAM_CSIS3");

	fimc_is_enable(dev, "GATE_IS_CAM_CSISX4_DMA");
	fimc_is_enable(dev, "GATE_IS_CAM_QE_CSISX4");

	return  0;
}

int exynos8895_fimc_is_sensor_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	int ret = 0;

	switch (channel) {
	case 0:
		exynos8895_fimc_is_csi_gate(dev, 1, true);
		exynos8895_fimc_is_csi_gate(dev, 2, true);
		exynos8895_fimc_is_csi_gate(dev, 3, true);
		break;
	case 1:
		exynos8895_fimc_is_csi_gate(dev, 0, true);
		exynos8895_fimc_is_csi_gate(dev, 2, true);
		exynos8895_fimc_is_csi_gate(dev, 3, true);
		break;
	case 2:
		exynos8895_fimc_is_csi_gate(dev, 0, true);
		exynos8895_fimc_is_csi_gate(dev, 1, true);
		exynos8895_fimc_is_csi_gate(dev, 3, true);
		break;
	case 3:
		exynos8895_fimc_is_csi_gate(dev, 0, true);
		exynos8895_fimc_is_csi_gate(dev, 1, true);
		exynos8895_fimc_is_csi_gate(dev, 2, true);
		break;
	default:
		pr_err("channel is invalid(%d)\n", channel);
		ret = -EINVAL;
		goto p_err;
		break;
	}

p_err:
	return ret;
}

int exynos8895_fimc_is_sensor_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos8895_fimc_is_csi_gate(dev, channel, true);

	fimc_is_disable(dev, "GATE_IS_CAM_QE_CSISX4");
	fimc_is_disable(dev, "GATE_IS_CAM_CSISX4_DMA");

	return 0;
}

int exynos8895_fimc_is_sensor_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);


	snprintf(sclk_name, sizeof(sclk_name), "CIS_CLK%d", channel);

	fimc_is_enable(dev, sclk_name);
	fimc_is_set_rate(dev, sclk_name, 26 * 1000000);

	return 0;
}

int exynos8895_fimc_is_sensor_mclk_off(struct device *dev,
		u32 scenario,
		u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "CIS_CLK%d", channel);

	fimc_is_disable(dev, sclk_name);

	return 0;
}
#elif defined(CONFIG_SOC_EXYNOS8890)
static int exynos8890_fimc_is_csi_gate(struct device *dev, u32 instance, bool mask)
{
	int ret = 0;

	pr_debug("%s(instance : %d / mask : %d)\n", __func__, instance, mask);

	switch (instance) {
	case 0:
		if (mask) {
			fimc_is_disable(dev, "gate_csis0");
			fimc_is_disable(dev, "pxmxdx_csis0");
			fimc_is_disable(dev, "hs0_csis0_rx_byte");
			fimc_is_disable(dev, "hs1_csis0_rx_byte");
			fimc_is_disable(dev, "hs2_csis0_rx_byte");
			fimc_is_disable(dev, "hs3_csis0_rx_byte");
		} else {
			fimc_is_enable(dev, "gate_csis0");
			fimc_is_enable(dev, "pxmxdx_csis0");
			fimc_is_enable(dev, "hs0_csis0_rx_byte");
			fimc_is_enable(dev, "hs1_csis0_rx_byte");
			fimc_is_enable(dev, "hs2_csis0_rx_byte");
			fimc_is_enable(dev, "hs3_csis0_rx_byte");
		}
		break;
	case 1:
		if (mask) {
			fimc_is_disable(dev, "gate_csis1");
			fimc_is_disable(dev, "pxmxdx_csis1");
			fimc_is_disable(dev, "hs0_csis1_rx_byte");
			fimc_is_disable(dev, "hs1_csis1_rx_byte");
		} else {
			fimc_is_enable(dev, "gate_csis1");
			fimc_is_enable(dev, "pxmxdx_csis1");
			fimc_is_enable(dev, "hs0_csis1_rx_byte");
			fimc_is_enable(dev, "hs1_csis1_rx_byte");
		}
		break;
	case 2:
		if (mask) {
			fimc_is_disable(dev, "gate_csis2");
			fimc_is_disable(dev, "cam1_csis2");
			fimc_is_disable(dev, "cam1_phy0_csis2");
			fimc_is_disable(dev, "cam1_phy1_csis2");
			fimc_is_disable(dev, "cam1_phy2_csis2");
			fimc_is_disable(dev, "cam1_phy3_csis2");
		} else {
			fimc_is_enable(dev, "gate_csis2");
			fimc_is_enable(dev, "cam1_csis2");
			fimc_is_enable(dev, "cam1_phy0_csis2");
			fimc_is_enable(dev, "cam1_phy1_csis2");
			fimc_is_enable(dev, "cam1_phy2_csis2");
			fimc_is_enable(dev, "cam1_phy3_csis2");
		}
		break;
	case 3:
		if (mask) {
			fimc_is_disable(dev, "gate_csis3");
			fimc_is_disable(dev, "cam1_csis3");
			fimc_is_disable(dev, "cam1_phy0_csis3");
		} else {
			fimc_is_enable(dev, "gate_csis3");
			fimc_is_enable(dev, "cam1_csis3");
			fimc_is_enable(dev, "cam1_phy0_csis3");
		}
		break;
	default:
		pr_err("(%s) instance is invalid(%d)\n", __func__, instance);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int exynos8890_fimc_is_sensor_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
	/* this dummy enable make refcount to be 1 for clock off */
	fimc_is_enable(dev, "gate_csis0");
	fimc_is_enable(dev, "gate_csis1");
	fimc_is_enable(dev, "pxmxdx_csis0");
	fimc_is_enable(dev, "pxmxdx_csis1");
	fimc_is_enable(dev, "pxmxdx_trex");
	fimc_is_enable(dev, "hs0_csis0_rx_byte");
	fimc_is_enable(dev, "hs1_csis0_rx_byte");
	fimc_is_enable(dev, "hs2_csis0_rx_byte");
	fimc_is_enable(dev, "hs3_csis0_rx_byte");
	fimc_is_enable(dev, "hs0_csis1_rx_byte");
	fimc_is_enable(dev, "hs1_csis1_rx_byte");

	fimc_is_enable(dev, "gate_csis2");
	fimc_is_enable(dev, "gate_csis3");
	fimc_is_enable(dev, "cam1_csis2");
	fimc_is_enable(dev, "cam1_csis3");
	fimc_is_enable(dev, "cam1_trex");
	fimc_is_enable(dev, "cam1_bus");
	fimc_is_enable(dev, "cam1_phy0_csis2");
	fimc_is_enable(dev, "cam1_phy1_csis2");
	fimc_is_enable(dev, "cam1_phy2_csis2");
	fimc_is_enable(dev, "cam1_phy3_csis2");
	fimc_is_enable(dev, "cam1_phy0_csis3");

	return  0;
}

int exynos8890_fimc_is_sensor_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	int ret = 0;

	switch (channel) {
	case 0:
		/* CSI */
		exynos8890_fimc_is_csi_gate(dev, 1, true);
		exynos8890_fimc_is_csi_gate(dev, 2, true);
		exynos8890_fimc_is_csi_gate(dev, 3, true);
		break;
	case 1:
		/* CSI */
		exynos8890_fimc_is_csi_gate(dev, 0, true);
		exynos8890_fimc_is_csi_gate(dev, 2, true);
		exynos8890_fimc_is_csi_gate(dev, 3, true);
		break;
	case 2:
		/* CSI */
		/* TODO : Disable BYTE Clock for back */
		/* exynos8890_fimc_is_csi_gate(dev, 0, true); */
		exynos8890_fimc_is_csi_gate(dev, 1, true);
		exynos8890_fimc_is_csi_gate(dev, 3, true);
		break;
	case 3:
		/* CSI */
		exynos8890_fimc_is_csi_gate(dev, 0, true);
		exynos8890_fimc_is_csi_gate(dev, 1, true);
		exynos8890_fimc_is_csi_gate(dev, 2, true);
		break;
	default:
		pr_err("channel is invalid(%d)\n", channel);
		ret = -EINVAL;
		goto p_err;
		break;
	}

p_err:
	return ret;
}

int exynos8890_fimc_is_sensor_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	int ret = 0;

	/* CSI */
	exynos8890_fimc_is_csi_gate(dev, channel, true);

	if (channel == 2)
		/* TODO : Don't need to care about BYTE Clock gating */
		exynos8890_fimc_is_csi_gate(dev, 0, true);

	fimc_is_disable(dev, "pxmxdx_trex");
	fimc_is_disable(dev, "cam1_trex");
	fimc_is_disable(dev, "cam1_bus");

	if (scenario == SENSOR_SCENARIO_NORMAL)
		goto p_err;

	switch (channel) {
	case 0:
		/* BUS0 */
		fimc_is_disable(dev, "gate_aclk_lh_cam0");

		/* CAM0 */
		fimc_is_disable(dev, "mout_user_mux_aclk_cam0_csis0_690");
		fimc_is_disable(dev, "mout_user_mux_aclk_cam0_bnsa_690");
		fimc_is_disable(dev, "mout_user_mux_aclk_cam0_bnsd_690");
		fimc_is_disable(dev, "mout_user_mux_aclk_cam0_trex_532");
		fimc_is_disable(dev, "mout_user_mux_aclk_cam0_nocp_133");
		fimc_is_disable(dev, "mout_user_mux_phyclk_rxbyteclkhs0_s4");
		fimc_is_disable(dev, "mout_user_mux_phyclk_rxbyteclkhs1_s4");
		fimc_is_disable(dev, "mout_user_mux_phyclk_rxbyteclkhs2_s4");
		fimc_is_disable(dev, "mout_user_mux_phyclk_rxbyteclkhs3_s4");
		break;
	case 1:
		/* BUS0 */
		fimc_is_disable(dev, "gate_aclk_lh_cam0");

		/* CAM0 */
		fimc_is_disable(dev, "mout_user_mux_aclk_cam0_csis1_174");
		fimc_is_disable(dev, "mout_user_mux_aclk_cam0_bnsb_690");
		fimc_is_disable(dev, "mout_user_mux_aclk_cam0_trex_532");
		fimc_is_disable(dev, "mout_user_mux_aclk_cam0_nocp_133");
		fimc_is_disable(dev, "mout_user_mux_phyclk_rxbyteclkhs0_s2a");
		break;
	case 2:
		/* BUS0 */
		fimc_is_enable(dev, "gate_aclk_lh_cam1");

		/* CAM1 */
		fimc_is_disable(dev, "mout_user_mux_aclk_cam1_bnscsis_133");
		fimc_is_disable(dev, "mout_user_mux_aclk_cam1_trex_532");
		fimc_is_disable(dev, "mout_user_mux_aclk_cam1_nocp_133");
		fimc_is_disable(dev, "mout_user_mux_phyclk_hs0_csis2_rx_byte");
		fimc_is_disable(dev, "mout_user_mux_aclk_cam1_busperi_334");
		break;
	default:
		pr_err("channel is invalid(%d)\n", channel);
		ret = -EINVAL;
		goto p_err;
		break;
	}

p_err:
	return ret;
}

int exynos8890_fimc_is_sensor_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "isp_sensor%d", channel);

	fimc_is_enable(dev, sclk_name);
	fimc_is_set_rate(dev, sclk_name, 26 * 1000000);

	return 0;
}

int exynos8890_fimc_is_sensor_mclk_off(struct device *dev,
		u32 scenario,
		u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "isp_sensor%d", channel);

	fimc_is_disable(dev, sclk_name);

	return 0;
}
#elif defined(CONFIG_SOC_EXYNOS7570)
int exynos7570_fimc_is_sensor_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
	return  0;
}

int exynos7570_fimc_is_sensor_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	int ret = 0;

	/* CSI */
	fimc_is_enable(dev, "isp_cam");
	fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");

	return ret;
}

int exynos7570_fimc_is_sensor_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	int ret = 0;

	/* CSI */
	fimc_is_disable(dev, "isp_cam");
	fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");

	return ret;
}

int exynos7570_fimc_is_sensor_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "mif_isp_sensor%d", channel);

	fimc_is_enable(dev, sclk_name);
	fimc_is_set_rate(dev, sclk_name, 26 * 1000000);

	return 0;
}

int exynos7570_fimc_is_sensor_mclk_off(struct device *dev,
		u32 scenario,
		u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "mif_isp_sensor%d", channel);

	fimc_is_disable(dev, sclk_name);

	return 0;
}
#elif defined(CONFIG_SOC_EXYNOS7870)
static int exynos7870_fimc_is_csi_gate(struct device *dev, u32 instance, bool mask)
{
	int ret = 0;

	pr_debug("%s(instance : %d / mask : %d)\n", __func__, instance, mask);

	switch (instance) {
	case 0:
		if (mask) {
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");
		} else {
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");
		}
		break;
	case 1:
		if (mask) {
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user");
		} else {
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user");
		}
		break;
	default:
		pr_err("(%s) instance is invalid(%d)\n", __func__, instance);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int exynos7870_fimc_is_sensor_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
	return  0;
}

int exynos7870_fimc_is_sensor_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	int ret = 0;
	fimc_is_enable(dev, "gate_isp_cam");
	fimc_is_enable(dev, "pxmxdx_isp_cam");

	switch (channel) {
	case 0:
		/* CSI */
		exynos7870_fimc_is_csi_gate(dev, 0, false);
		break;
	case 1:
		/* CSI */
		exynos7870_fimc_is_csi_gate(dev, 1, false);
		break;
	default:
		pr_err("channel is invalid(%d)\n", channel);
		ret = -EINVAL;
		goto p_err;
		break;
	}

p_err:
	return ret;
}

int exynos7870_fimc_is_sensor_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	int ret = 0;

	fimc_is_disable(dev, "gate_isp_cam");
	fimc_is_disable(dev, "pxmxdx_isp_cam");
	/* CSI */
	exynos7870_fimc_is_csi_gate(dev, channel, true);

	return ret;
}

int exynos7870_fimc_is_sensor_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "isp_sensor%d_sclk", channel);

	fimc_is_enable(dev, sclk_name);
	fimc_is_set_rate(dev, sclk_name, 26 * 1000000);

	return 0;
}

int exynos7870_fimc_is_sensor_mclk_off(struct device *dev,
		u32 scenario,
		u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "isp_sensor%d_sclk", channel);

	fimc_is_disable(dev, sclk_name);

	return 0;
}

#elif defined(CONFIG_SOC_EXYNOS7880)
static int exynos7880_fimc_is_csi_gate(struct device *dev, u32 instance, bool mask)
{
	int ret = 0;

	pr_debug("%s(instance : %d / mask : %d)\n", __func__, instance, mask);

	switch (instance) {
	case 0:
		if (mask) {
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs1_s4_user");
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs2_s4_user");
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs3_s4_user");
		} else {
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs1_s4_user");
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs2_s4_user");
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs3_s4_user");
		}
		break;
	case 1:
		if (mask) {
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user");
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs1_s4s_user");
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs2_s4s_user");
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs3_s4s_user");
		} else {
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user");
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs1_s4s_user");
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs2_s4s_user");
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs3_s4s_user");
		}
		break;
	default:
		pr_err("(%s) instance is invalid(%d)\n", __func__, instance);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int exynos7880_fimc_is_sensor_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
	return  0;
}

int exynos7880_fimc_is_sensor_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	int ret = 0;

	switch (channel) {
	case 0:
		/* CSI */
		exynos7880_fimc_is_csi_gate(dev, 0, false);
		break;
	case 1:
		/* CSI */
		exynos7880_fimc_is_csi_gate(dev, 1, false);
		break;
	default:
		pr_err("channel is invalid(%d)\n", channel);
		ret = -EINVAL;
		goto p_err;
		break;
	}

#if defined(CONFIG_VENDER_PSV)
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
#endif

p_err:
	return ret;
}

int exynos7880_fimc_is_sensor_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	int ret = 0;

	/* CSI */
	exynos7880_fimc_is_csi_gate(dev, channel, true);

#if defined(CONFIG_VENDER_PSV)
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
#endif

	return ret;
}

int exynos7880_fimc_is_sensor_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "isp_sensor%d_sclk", channel);

	fimc_is_enable(dev, sclk_name);
	fimc_is_set_rate(dev, sclk_name, 26 * 1000000);

	return 0;
}

int exynos7880_fimc_is_sensor_mclk_off(struct device *dev,
		u32 scenario,
		u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "isp_sensor%d_sclk", channel);

	fimc_is_disable(dev, sclk_name);

	return 0;
}
#endif

int exynos_fimc_is_sensor_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
#if defined(CONFIG_SOC_EXYNOS8895)
	exynos8895_fimc_is_sensor_iclk_cfg(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS8890)
	exynos8890_fimc_is_sensor_iclk_cfg(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7570)
	exynos7570_fimc_is_sensor_iclk_cfg(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7870)
	exynos7870_fimc_is_sensor_iclk_cfg(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7880)
	exynos7880_fimc_is_sensor_iclk_cfg(dev, scenario, channel);
#else
#error exynos_fimc_is_sensor_iclk_cfg is not implemented
#endif
	return 0;
}

int exynos_fimc_is_sensor_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
#if defined(CONFIG_SOC_EXYNOS8895)
	exynos8895_fimc_is_sensor_iclk_on(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS8890)
	exynos8890_fimc_is_sensor_iclk_on(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7570)
	exynos7570_fimc_is_sensor_iclk_on(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7870)
	exynos7870_fimc_is_sensor_iclk_on(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7880)
	exynos7880_fimc_is_sensor_iclk_on(dev, scenario, channel);
#else
#error exynos_fimc_is_sensor_iclk_on is not implemented
#endif
	return 0;
}

int exynos_fimc_is_sensor_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
#if defined(CONFIG_SOC_EXYNOS8895)
	exynos8895_fimc_is_sensor_iclk_off(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS8890)
	exynos8890_fimc_is_sensor_iclk_off(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7570)
	exynos7570_fimc_is_sensor_iclk_off(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7870)
	exynos7870_fimc_is_sensor_iclk_off(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7880)
	exynos7880_fimc_is_sensor_iclk_off(dev, scenario, channel);
#else
#error exynos_fimc_is_sensor_iclk_off is not implemented
#endif
	return 0;
}

int exynos_fimc_is_sensor_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
#if defined(CONFIG_SOC_EXYNOS8895)
	exynos8895_fimc_is_sensor_mclk_on(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS8890)
	exynos8890_fimc_is_sensor_mclk_on(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7570)
	exynos7570_fimc_is_sensor_mclk_on(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7870)
	exynos7870_fimc_is_sensor_mclk_on(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7880)
	exynos7880_fimc_is_sensor_mclk_on(dev, scenario, channel);
#else
#error exynos_fimc_is_sensor_mclk_on is not implemented
#endif
	return 0;
}

int exynos_fimc_is_sensor_mclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
#if defined(CONFIG_SOC_EXYNOS8895)
	exynos8895_fimc_is_sensor_mclk_off(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS8890)
	exynos8890_fimc_is_sensor_mclk_off(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7570)
	exynos7570_fimc_is_sensor_mclk_off(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7870)
	exynos7870_fimc_is_sensor_mclk_off(dev, scenario, channel);
#elif defined(CONFIG_SOC_EXYNOS7880)
	exynos7880_fimc_is_sensor_mclk_off(dev, scenario, channel);
#else
#error exynos_fimc_is_sensor_mclk_off is not implemented
#endif
	return 0;
}
