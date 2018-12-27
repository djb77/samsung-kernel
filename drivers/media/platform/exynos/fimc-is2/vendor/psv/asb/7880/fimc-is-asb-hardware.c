/*
* Samsung Exynos SoC series FIMC-IS driver
 *
 * Exynos fimc-is ASB functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include "fimc-is-core.h"
#include "fimc-is-asb.h"

#define SYSREG_ISP_USER_CON	0x144F1000
#define SYSREG_ISP_MIPIPHY_CON	0x144F1040
#define PMU_MIPI_PHY_M4S2_CONTROL	0x106B070C
#define	FIMC_BNS_CIGCTRL	0x14410004

int fimc_is_set_asb_input_path(struct fimc_is_core *core, int id)
{
	int ret = 0;
	int gpio_switch = 0;
	struct device *dev;
	struct device_node *dnode;
	void __iomem *isp_mipiphy_con;
	void __iomem *isp_user_con;
	void __iomem *pmu_phy_con;
	void __iomem *bns_format;
	u32 val;

	dev = &core->pdev->dev;
	dnode = dev->of_node;

	/* set the BNS format as RAW10 becuase BNS doesn't support YUV format */
	bns_format = ioremap(FIMC_BNS_CIGCTRL, SZ_4K);
	val = readl(bns_format);
	val &= ~(0x3F << 24);
	val |= 0x2B << 24;
	writel(val, bns_format);
	iounmap(bns_format);


	if (id == 0) {
		gpio_switch = of_get_named_gpio(dnode, "gpio_asb_switch", 0);
		if (!gpio_is_valid(gpio_switch)) {
			dev_err(dev, "failed to set switch\n");
			return -EINVAL;
		} else {
			gpio_request_one(gpio_switch, GPIOF_OUT_INIT_HIGH, "ASB_switch_set_1");
			gpio_free(gpio_switch);
		}

		/* select CSIS0 for BNS input */
		isp_user_con = ioremap(SYSREG_ISP_USER_CON, SZ_4K);
		val = readl(isp_user_con);
		val &= ~(1 << 3);
		writel(val, isp_user_con);
		iounmap(isp_user_con);
	} else if (id == 1) {
		/* select M0S4 for CSIS1 */
		isp_mipiphy_con = ioremap(SYSREG_ISP_MIPIPHY_CON, SZ_4K);
		val = readl(isp_mipiphy_con);
		val |= (1 << 3);
		writel(val, isp_mipiphy_con);
		iounmap(isp_mipiphy_con);

		/* select CSIS1 for BNS input */
		isp_user_con = ioremap(SYSREG_ISP_USER_CON, SZ_4K);
		val = readl(isp_user_con);
		val |= (1 << 3);
		writel(val, isp_user_con);
		iounmap(isp_user_con);
	} else if (id == 2) {
		gpio_switch = of_get_named_gpio(dnode, "gpio_asb_switch", 0);
		if (!gpio_is_valid(gpio_switch)) {
			dev_err(dev, "failed to set switch\n");
			return -EINVAL;
		} else {
			gpio_request_one(gpio_switch, GPIOF_OUT_INIT_LOW, "ASB_switch_set_0");
			gpio_free(gpio_switch);
		}

		/* select & reset M4S2 for CSIS1 */
		isp_mipiphy_con = ioremap(SYSREG_ISP_MIPIPHY_CON, SZ_4K);
		val = readl(isp_mipiphy_con);
		val &= ~(1 << 3);
		val |= (1 << 2);
		writel(val, isp_mipiphy_con);
		iounmap(isp_mipiphy_con);

		/* select CSIS1 for BNS input */
		isp_user_con = ioremap(SYSREG_ISP_USER_CON, SZ_4K);
		val = readl(isp_user_con);
		val |= (1 << 3);
		writel(val, isp_user_con);
		iounmap(isp_user_con);

		/* M4S2 PMU isolation */
		pmu_phy_con = ioremap(PMU_MIPI_PHY_M4S2_CONTROL, SZ_4K);
		writel(1, pmu_phy_con);
		iounmap(pmu_phy_con);
	}



	return ret;
}

int fimc_is_set_cam_dma_path(struct fimc_is_device_sensor *device, int id)
{
	int ret = 0;
	struct fimc_is_device_flite *flite;
	struct fimc_is_device_csi *csi;

	flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(device->subdev_flite);
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(device->subdev_csi);

	if (id == 0) {
		set_bit(CSIS_DMA_ENABLE, &csi->state);
		clear_bit(FLITE_DMA_ENABLE, &flite->state);
	} else {
		clear_bit(CSIS_DMA_ENABLE, &csi->state);
		set_bit(FLITE_DMA_ENABLE, &flite->state);
		clear_bit(FLITE_DUMMY, &flite->state);
	}

	return ret;
}
