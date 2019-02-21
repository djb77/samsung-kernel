/*
 * PCIe phy driver for Samsung EXYNOS8890
 *
 * Copyright (C) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Kyoungil Kim <ki0351.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/of_gpio.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/exynos-pci-noti.h>
#include "pcie-designware.h"
#include "pci-exynos.h"

#if IS_ENABLED(CONFIG_EXYNOS_OTP)
#include <linux/exynos_otp.h>
#endif
extern struct exynos_pcie g_pcie[MAX_RC_NUM];

/* avoid checking rx elecidle when access DBI */
void exynos_phy_check_rx_elecidle(void *phy_pcs_base_regs, int val)
{
	u32 reg_val;

	reg_val = readl(phy_pcs_base_regs + 0xEC);
	reg_val &= ~(0x1 << 3);
	reg_val |= (val << 3);
	writel(reg_val, phy_pcs_base_regs + 0xEC);
}

/* Need to apply it at the initial setting sequence */
void exynos_100Mhz_from_socpll_off(void)
{
/* It is already setted in bootloader */
#if 0
	/* FSYS1 CMU Clock Gating Enable */
	writel(0x10000000, 0x11400800);

	/* PCIe QCH SOCPLL gating */
	writel(0x00000000, 0x11403000);
#endif
	return;
}

/* PHY all power down */
void exynos_phy_all_pwrdn(void *phy_base_regs, void *sysreg_base_regs, int ch_num)
{
/* if you want to use channel 1, you need to set below register */
#if 0
	u32 val;
#endif

	/* Trsv */
	if (ch_num == 0)
		writel(0xFE, phy_base_regs + (0x57 * 4));
	else

		writel(0xFE, phy_base_regs + (0xA7 * 4));

	/* Common */
	writel(0xC1, phy_base_regs + (0x20 * 4));

/* if you want to use channel 1, you need to set below register */
#if 0
	/* Ch1 SOC Block and PCS Lane Disable */
	val = readl(sysreg_base_regs + 0xC);
	val &= ~(0xA << 12);
	writel(val, sysreg_base_regs + 0xC);
#endif
	exynos_100Mhz_from_socpll_off();
}

/* PHY all power down clear */
void exynos_phy_all_pwrdn_clear(void *phy_base_regs, void *sysreg_base_regs, int ch_num)
{
/* if you want to use channel 1, you need to set below register */
#if 0
	u32 val;

	/* Ch0 & Ch1 SOC Block and PCS Lane Enable */
	val = readl(sysreg_base_regs + 0xC);
	val |= 0xf << 12;
	writel(val, sysreg_base_regs + 0xC);
#endif

	/* Trsv */
	if (ch_num == 0)
		writel(0xC0, phy_base_regs + (0x20 * 4));
	else
		writel(0x7E, phy_base_regs + (0x57 * 4));

	/* Common */
	writel(0x7E, phy_base_regs + (0xA7 * 4));
}

void exynos_pcie_phy_otp_config(void *phy_base_regs, int ch_num)
{
#if IS_ENABLED(CONFIG_EXYNOS_OTP)
	u8 utype;
	u8 uindex_count;
	struct tune_bits *data;
	u32 index;
	u32 value;
	u16 magic_code;
	u32 i;

	struct pcie_port *pp = &g_pcie[ch_num].pp;

	if (ch_num == 0)
		magic_code = 0x5030;
	else if (ch_num == 1)
		magic_code = 0x5031;
	else
		return;

	if (otp_tune_bits_parsed(magic_code, &utype, &uindex_count, &data) == 0) {
		dev_err(pp->dev, "%s: [OTP] uindex_count %d", __func__, uindex_count);
		for (i = 0; i < uindex_count; i++) {
			index = data[i].index;
			value = data[i].value;

			dev_err(pp->dev, "%s: [OTP][Return Value] index = 0x%x, value = 0x%x\n", __func__, index, value);
			dev_err(pp->dev, "%s: [OTP][Before Reg Value] offset 0x%x = 0x%x\n", __func__, index * 4, readl(phy_base_regs + (index * 4)));
			if (readl(phy_base_regs + (index * 4)) != value)
				writel(value, phy_base_regs + (index * 4));
			else
				return;
			dev_err(pp->dev, "%s: [OTP][After Reg Value] offset 0x%x = 0x%x\n", __func__, index * 4, readl(phy_base_regs + (index * 4)));
		}
	}
#else
	return;
#endif
}

void exynos_pcie_phy_config(void *phy_base_regs, void *phy_pcs_base_regs, void *sysreg_base_regs, void *elbi_base_regs, int ch_num)
{
	/* 26MHz gen1 */
	u32 cmn_config_val[48] = { 0x01, 0xE1, 0x05, 0x00, 0x88, 0x88, 0x88, 0x04, 0x31, 0x45, 0x65, 0x24, 0x33, 0x08, 0xA4, 0xFC,
				   0xC6, 0x05, 0xE6, 0x80, 0x00, 0x00, 0x00, 0x00, 0x60, 0x11, 0x00, 0x90, 0x10, 0x04, 0x4E, 0x8E,
				   /* Gen1 */
				   0xC0, 0xFF, 0x9B, 0x52, 0x22, 0x73, 0x47, 0x8C, 0x40, 0x00, 0x22, 0xFF, 0xFF, 0xFF, 0x00, 0x80 };
	u32 trsv_config_val[156] = { /* Ch0 */
				    0x31, 0x40, 0x37, 0x99, 0x85, 0x00, 0xC0, 0xFF, 0xFF, 0x3F, 0x8C, 0xC8, 0x02, 0x01, 0x88, 0x80,
				    0x06, 0x90, 0x68, 0x66, 0x09, 0x31, 0x42, 0x44, 0xC6, 0x18, 0x03, 0x33, 0x58, 0xE7, 0x20, 0x22,
				    0x80, 0x38, 0x05, 0x85, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x55, 0x15, 0xAC, 0xAA, 0x3E, 0x00,
				    0x00, 0x00, 0x00, 0x3F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x85, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
				     /* Ch1 */
				    0x81, 0x40, 0x37, 0x99, 0x85, 0x00, 0xC0, 0xFF, 0xFF, 0x3F, 0x8C, 0xC8, 0x02, 0x01, 0x88, 0x80,
				    0x06, 0x90, 0x68, 0x66, 0x09, 0x31, 0x42, 0x44, 0xC6, 0x18, 0x03, 0x33, 0x58, 0xE7, 0x20, 0x22,
				    0x80, 0x38, 0x05, 0x85, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x55, 0x15, 0xAC, 0xAA, 0x3E, 0x00,
				    0x00, 0x00, 0x00, 0x3F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				     /* Gen1 */
				    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x85, 0xFF, 0x00, };
	int i;
	u32 reg_val;

	writel(readl(sysreg_base_regs) & ~(0x1 << 1), sysreg_base_regs);
	writel((((readl(sysreg_base_regs + 0xC) & ~(0xf << 4)) & ~(0xf << 2)) | (0x3 << 2)) & ~(0x1 << 1), sysreg_base_regs + 0xC);

	/* PCIE_MAC RST */
	writel(0x0, elbi_base_regs + 0x290);
	udelay(10);
	/* PCIE_PHY PCS&PMA(CMN)_RST */
	writel(0x0, elbi_base_regs + 0x28C);
	udelay(10);
	/* pcs_g_rst */
	writel(0x1, elbi_base_regs + 0x288);
	udelay(10);
	writel(0x0, elbi_base_regs + 0x288);
	udelay(10);
	writel(0x1, elbi_base_regs + 0x288);
	udelay(10);

	/* PHY Common block Setting */
	for (i = 0; i < 48; i++)
		writel(cmn_config_val[i], phy_base_regs + (i * 4));

	/* PHY Tranceiver/Receiver block Setting */
	for (i = 0; i < 156; i++)
		writel(trsv_config_val[i], phy_base_regs + ((0x30 + i) * 4));

#if IS_ENABLED(CONFIG_EXYNOS_OTP)
	/* PHY OTP Tuning bit configuration Setting */
	exynos_pcie_phy_otp_config(phy_base_regs, ch_num);
#endif

	/* tx amplitude control */
	writel(0x14, phy_base_regs + (0x5C * 4));

	/* tx latency */
	writel(0x70, phy_pcs_base_regs + 0xF8);
	/* pcs refclk out control */
	writel(0x87, phy_pcs_base_regs + 0x100);
	writel(0x50, phy_pcs_base_regs + 0x104);

	/* PRGM_TIMEOUT_L1SS_VAL Setting */
	reg_val = readl(phy_pcs_base_regs + 0xC);
	reg_val &= ~(0x1 << 1);
	reg_val |= (0x1 << 4);
	writel(reg_val, phy_pcs_base_regs + 0xC);

	/* PCIE_MAC RST */
	writel(0x1, elbi_base_regs + 0x290);
	udelay(10);

	/* PCIE_PHY PCS&PMA(CMN)_RST */
	writel(0x1, elbi_base_regs + 0x28C);
	udelay(10);

	/* Re-write at Kangchen only */
	/* PHY Common block Setting */
	for (i = 0; i < 48; i++)
		writel(cmn_config_val[i], phy_base_regs + (i * 4));
	/* PHY Tranceiver/Receiver block Setting */
	for (i = 0; i < 156; i++)
		writel(trsv_config_val[i], phy_base_regs + ((0x30 + i) * 4));

}

void exynos_pcie_phy_init(struct pcie_port *pp)
{
	struct exynos_pcie *exynos_pcie = to_exynos_pcie(pp);

	dev_info(pp->dev, "Initialize PHY functions.\n");

	exynos_pcie->phy_ops.phy_check_rx_elecidle =
		exynos_phy_check_rx_elecidle;
	exynos_pcie->phy_ops.phy_all_pwrdn = exynos_phy_all_pwrdn;
	exynos_pcie->phy_ops.phy_all_pwrdn_clear = exynos_phy_all_pwrdn_clear;
	exynos_pcie->phy_ops.phy_config = exynos_pcie_phy_config;
}

static void quirk_brcm_header(struct pci_dev *dev)
{
	/* Change BAR2 Size from 4MB to 3MB */
	pci_resource_end(dev, 2) = 0x300000 - 1;
}
DECLARE_PCI_FIXUP_HEADER(PCI_VENDOR_ID_BROADCOM, PCI_ANY_ID, quirk_brcm_header);

static void quirk_brcm_enable(struct pci_dev *dev)
{
	int count = 0;
	u32 reg;

	pr_info("Before changing BAR0\n");
	pci_read_config_dword(dev, PCI_COMMAND, &reg);
	pr_info("CMD Register = 0x%x, ", reg);
	pci_read_config_dword(dev, PCI_BASE_ADDRESS_0, &reg);
	pr_info("BAR0 = 0x%x\n", reg);

	pr_info("Change BAR0 to fit 0x11C00000\n");
	pci_write_config_dword(dev, PCI_BASE_ADDRESS_0, 0x11C00000);

	/* retry writing of base address */
	pr_info("BAR0 retry code added.\n");
	while (1) {

		pci_read_config_dword(dev, PCI_BASE_ADDRESS_0, &reg);
		reg &= 0xfffffff0;
		if (reg == 0x11C00000)
			break;
		pr_info("BAR0 resizing retry - %d %x\n", ++count, reg);
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_0, 0x11C00000);
		if (count > 100)
			break;
	}

	pr_info("After changing BAR0\n");
	pci_read_config_dword(dev, PCI_COMMAND, &reg);
	pr_info("CMD Register = 0x%x, ", reg);
	pci_read_config_dword(dev, PCI_BASE_ADDRESS_0, &reg);
	pr_info("BAR0 = 0x%x\n", reg);
}
DECLARE_PCI_FIXUP_ENABLE(PCI_VENDOR_ID_BROADCOM, PCI_ANY_ID, quirk_brcm_enable);

static void quirk_pci_async_suspend(struct pci_dev *dev)
{
	device_disable_async_suspend(&dev->dev);
	printk("[%s] async suspend disabled\n", __func__);
}
DECLARE_PCI_FIXUP_FINAL(PCI_ANY_ID, PCI_ANY_ID, quirk_pci_async_suspend);
