/*
 * PCIe clock control driver for Samsung EXYNOS7420
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Kyoungil Kim <ki0351.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

static int exynos_pcie_clock_get(struct pcie_port *pp)
{
	struct exynos_pcie *exynos_pcie = to_exynos_pcie(pp);
	struct exynos_pcie_clks	*clks = &exynos_pcie->clks;
	int i;

	if (exynos_pcie->ch_num == 0) {
		clks->pcie_clks[0] = devm_clk_get(pp->dev, "gate_pciewifi0");
		clks->phy_clks[0] = devm_clk_get(pp->dev, "wifi0_dig_refclk");
		clks->phy_clks[1] = devm_clk_get(pp->dev, "pcie_wifi0_tx0");
		clks->phy_clks[2] = devm_clk_get(pp->dev, "pcie_wifi0_rx0");
	} else if (exynos_pcie->ch_num == 1) {
		clks->pcie_clks[0] = devm_clk_get(pp->dev, "gate_pciewifi1");
		clks->phy_clks[0] = devm_clk_get(pp->dev, "wifi1_dig_refclk");
		clks->phy_clks[1] = devm_clk_get(pp->dev, "pcie_wifi1_tx0");
		clks->phy_clks[2] = devm_clk_get(pp->dev, "pcie_wifi1_rx0");
	}

	for (i = 0; i < exynos_pcie->pcie_clk_num; i++) {
		if (IS_ERR(clks->pcie_clks[i])) {
			dev_err(pp->dev, "Failed to get pcie clock\n");
			return -ENODEV;
		}
	}
	for (i = 0; i < exynos_pcie->phy_clk_num; i++) {
		if (IS_ERR(clks->phy_clks[i])) {
			dev_err(pp->dev, "Failed to get pcie clock\n");
			return -ENODEV;
		}
	}
	return 0;
}

static int exynos_pcie_clock_enable(struct pcie_port *pp, int enable)
{
	struct exynos_pcie *exynos_pcie = to_exynos_pcie(pp);
	struct exynos_pcie_clks	*clks = &exynos_pcie->clks;
	int i;

	if (enable) {
		for (i = 0; i < exynos_pcie->pcie_clk_num; i++)
			clk_prepare_enable(clks->pcie_clks[i]);
	} else {
		for (i = 0; i < exynos_pcie->pcie_clk_num; i++)
			clk_disable_unprepare(clks->pcie_clks[i]);
	}
	return 0;
}

static int exynos_pcie_phy_clock_enable(struct pcie_port *pp, int enable)
{
	struct exynos_pcie *exynos_pcie = to_exynos_pcie(pp);
	struct exynos_pcie_clks	*clks = &exynos_pcie->clks;
	int i;

	if (enable) {
		for (i = 0; i < exynos_pcie->phy_clk_num; i++)
			clk_prepare_enable(clks->phy_clks[i]);
	} else {
		for (i = 0; i < exynos_pcie->phy_clk_num; i++)
			clk_disable_unprepare(clks->phy_clks[i]);
	}
	return 0;
}
