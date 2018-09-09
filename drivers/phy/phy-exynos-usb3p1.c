/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Author: Sung-Hyun Na <sunghyun.na@samsung.com>
 *
 * Chip Abstraction Layer for USB PHY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include "phy-samsung-usb-cal.h"
#include "phy-exynos-usb3p1.h"
#include "phy-exynos-usb3p1-reg.h"

static void exynos_cal_usbphy_q_ch(void *regs_base, u8 enable)
{
	u32 phy_resume;

	if (enable) {
		/* WA for Q-channel: disable all q-act from usb */
		phy_resume = readl(regs_base + EXYNOS_USBCON_LINK_CTRL);
		phy_resume |= LINKCTRL_DIS_QACT_ID0;
		phy_resume |= LINKCTRL_DIS_QACT_VBUS_VALID;
		phy_resume |= LINKCTRL_DIS_QACT_BVALID;
		phy_resume |= LINKCTRL_DIS_QACT_LINKGATE;
		phy_resume &= ~LINKCTRL_FORCE_QACT;
		udelay(500);
		writel(phy_resume, regs_base + EXYNOS_USBCON_LINK_CTRL);
		udelay(500);
		phy_resume = readl(regs_base + EXYNOS_USBCON_LINK_CTRL);
		phy_resume |= LINKCTRL_FORCE_QACT;
		udelay(500);
		writel(phy_resume, regs_base + EXYNOS_USBCON_LINK_CTRL);
	} else {
		phy_resume = readl(regs_base + EXYNOS_USBCON_LINK_CTRL);
		phy_resume &= ~LINKCTRL_FORCE_QACT;
		phy_resume |= LINKCTRL_DIS_QACT_ID0;
		phy_resume |= LINKCTRL_DIS_QACT_VBUS_VALID;
		phy_resume |= LINKCTRL_DIS_QACT_BVALID;
		phy_resume |= LINKCTRL_DIS_QACT_LINKGATE;
		writel(phy_resume, regs_base + EXYNOS_USBCON_LINK_CTRL);
	}
}

static void link_vbus_filter_en(struct exynos_usbphy_info *info,
	u8 enable)
{
	u32 phy_resume;

	phy_resume = readl(info->regs_base + EXYNOS_USBCON_LINK_CTRL);
	if (enable)
		phy_resume &= ~LINKCTRL_BUS_FILTER_BYPASS_MASK;
	else
		phy_resume |= LINKCTRL_BUS_FILTER_BYPASS(0xf);
	writel(phy_resume, info->regs_base + EXYNOS_USBCON_LINK_CTRL);
}

static void phy_power_en(struct exynos_usbphy_info *info, u8 en)
{
	u32 reg;
	int main_version;

	main_version = info->version & EXYNOS_USBCON_VER_MAJOR_VER_MASK;
	if (main_version == EXYNOS_USBCON_VER_05_0_0) {
		void *__iomem reg_base;

		if (info->used_phy_port == 1)
			reg_base = info->regs_base_2nd;
		else
			reg_base = info->regs_base;

		/* 3.0 PHY Power Down control */
		reg = readl(reg_base + EXYNOS_USBCON_PWR);
		if (en)
			reg &= ~(PWR_TEST_POWERDOWN_SSP);
		else
			reg |= (PWR_TEST_POWERDOWN_SSP);
		writel(reg, reg_base + EXYNOS_USBCON_PWR);
	} else if (main_version == EXYNOS_USBCON_VER_03_0_0) {
		/* 2.0 PHY Power Down Control */
		reg = readl(info->regs_base + EXYNOS_USBCON_HSP_TEST);
		if (en)
			reg &= ~HSP_TEST_SIDDQ;
		else
			reg |= HSP_TEST_SIDDQ;
		writel(reg, info->regs_base + EXYNOS_USBCON_HSP_TEST);
	}
}

static void phy_sw_rst_high(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	int main_version;
	u32 clkrst;

	main_version = info->version & EXYNOS_USBCON_VER_MAJOR_VER_MASK;
	if ((main_version == EXYNOS_USBCON_VER_05_0_0) &&
			(info->used_phy_port == 1))
		regs_base = info->regs_base_2nd;

	clkrst = readl(regs_base + EXYNOS_USBCON_CLKRST);
	clkrst |= CLKRST_PHY_SW_RST;
	clkrst |= CLKRST_PHY_RST_SEL;
	writel(clkrst, regs_base + EXYNOS_USBCON_CLKRST);
}

static void phy_sw_rst_low(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	int main_version;
	u32 clkrst;

	main_version = info->version & EXYNOS_USBCON_VER_MAJOR_VER_MASK;
	if ((main_version == EXYNOS_USBCON_VER_05_0_0) &&
			(info->used_phy_port == 1))
		regs_base = info->regs_base_2nd;

	clkrst = readl(regs_base + EXYNOS_USBCON_CLKRST);
	clkrst |= CLKRST_PHY_RST_SEL;
	clkrst &= ~CLKRST_PHY_SW_RST;
	clkrst &= ~CLKRST_PORT_RST;
	writel(clkrst, regs_base + EXYNOS_USBCON_CLKRST);
}

void phy_exynos_usb_v3p1_pma_ready(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	u32 reg;

	reg = readl(regs_base + EXYNOS_USBCON_COMBO_PMA_CTRL);
	reg &= ~PMA_LOW_PWRN;
	writel(reg, regs_base + EXYNOS_USBCON_COMBO_PMA_CTRL);

	udelay(1);

	reg |= PMA_APB_SW_RST;
	reg |= PMA_INIT_SW_RST;
	reg |= PMA_CMN_SW_RST;
	reg |= PMA_TRSV_SW_RST;
	writel(reg, regs_base + EXYNOS_USBCON_COMBO_PMA_CTRL);

	udelay(1);

	reg &= ~PMA_APB_SW_RST;
	reg &= ~PMA_INIT_SW_RST;
	reg &= ~PMA_PLL_REF_REQ_MASK;
	reg &= ~PMA_REF_FREQ_MASK;
	reg |= PMA_REF_FREQ_SET(0x1);
	writel(reg, regs_base + EXYNOS_USBCON_COMBO_PMA_CTRL);

	reg = readl(regs_base + EXYNOS_USBCON_LINK_CTRL);
	reg &= ~LINKCTRL_PIPE3_FORCE_EN;
	writel(reg, regs_base + EXYNOS_USBCON_LINK_CTRL);
}

void phy_exynos_usb_v3p1_pma_sw_rst_release(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	u32 reg;

	/* Reset Release for USB/DP PHY */
	reg = readl(regs_base + EXYNOS_USBCON_COMBO_PMA_CTRL);
	reg &= ~PMA_CMN_SW_RST;
	reg &= ~PMA_TRSV_SW_RST;
	writel(reg, regs_base + EXYNOS_USBCON_COMBO_PMA_CTRL);

	udelay(200);

	/* Change pipe pclk to pipe3 */
	reg = readl(regs_base + EXYNOS_USBCON_CLKRST);
	reg |= CLKRST_LINK_PCLK_SEL;
	writel(reg, regs_base + EXYNOS_USBCON_CLKRST);
}

void phy_exynos_usb_v3p1_pipe_ready(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	u32 reg;

	reg = readl(regs_base + EXYNOS_USBCON_LINK_CTRL);
	reg &= ~LINKCTRL_PIPE3_FORCE_EN;
	writel(reg, regs_base + EXYNOS_USBCON_LINK_CTRL);
}

void phy_exynos_usb_v3p1_pipe_ovrd(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	u32 reg;

	/* force pipe3 signal for link */
	reg = readl(regs_base + EXYNOS_USBCON_LINK_CTRL);
	reg |= LINKCTRL_PIPE3_FORCE_EN;
	reg &= ~LINKCTRL_PIPE3_FORCE_PHY_STATUS;
	reg |= LINKCTRL_PIPE3_FORCE_RX_ELEC_IDLE;
	writel(reg, regs_base + EXYNOS_USBCON_LINK_CTRL);

	/* PMA Disable */
	reg = readl(regs_base + EXYNOS_USBCON_COMBO_PMA_CTRL);
	reg |= PMA_LOW_PWRN;
	writel(reg, regs_base + EXYNOS_USBCON_COMBO_PMA_CTRL);
}

void phy_exynos_usb3p1_rewa_ready(struct exynos_usbphy_info *info);
void phy_exynos_usb_v3p1_late_enable(struct exynos_usbphy_info *info);

void phy_exynos_usb_v3p1_enable(struct exynos_usbphy_info *info)
{
	void __iomem *regs_base = info->regs_base;
	u32 reg;
	u32 reg_hsp;
	bool ss_only_cap;
	int main_version;

	main_version = info->version & EXYNOS_USBCON_VER_MAJOR_VER_MASK;
	ss_only_cap = (info->version & EXYNOS_USBCON_VER_SS_CAP) >> 4;

	if (main_version == EXYNOS_USBCON_VER_03_0_0) {
		/* Set force q-channel */
		exynos_cal_usbphy_q_ch(regs_base, 1);

		/* Link Reset */
		if (main_version == EXYNOS_USBCON_VER_03_0_0) {
			reg = readl(info->regs_base + EXYNOS_USBCON_CLKRST);
			reg |= CLKRST_LINK_SW_RST;
			writel(reg, regs_base + EXYNOS_USBCON_CLKRST);

			udelay(10);

			reg &= ~CLKRST_LINK_SW_RST;
			writel(reg, regs_base + EXYNOS_USBCON_CLKRST);
		}
        }

	/* Set PHY Reset High */
	phy_sw_rst_high(info);

	/* Enable PHY Power Mode */
	phy_power_en(info, 1);

	if (!ss_only_cap) {
		reg = readl(regs_base + EXYNOS_USBCON_UTMI);
		reg &= ~UTMI_FORCE_SUSPEND;
		reg &= ~UTMI_FORCE_SLEEP;
		reg &= ~UTMI_DP_PULLDOWN;
		reg &= ~UTMI_DM_PULLDOWN;
		writel(reg, regs_base + EXYNOS_USBCON_UTMI);

		/* set phy clock & control HS phy */
		reg = readl(regs_base + EXYNOS_USBCON_HSP);
		reg |= HSP_EN_UTMISUSPEND;
		if (info->common_block_disable)
			reg |= HSP_COMMONONN;
		else
			reg &= ~HSP_COMMONONN;
		writel(reg, regs_base + EXYNOS_USBCON_HSP);
	} else {
		void *ss_reg_base;

		if (info->used_phy_port == 1)
			ss_reg_base = info->regs_base_2nd;
		else
			ss_reg_base = info->regs_base;
		/* Change pipe pclk to pipe3 */
		reg = readl(ss_reg_base + EXYNOS_USBCON_CLKRST);
		reg |= CLKRST_LINK_PCLK_SEL;
		writel(reg, ss_reg_base + EXYNOS_USBCON_CLKRST);
	}
	udelay(100);
	phy_sw_rst_low(info);

	if (ss_only_cap) {
		phy_exynos_usb_v3p1_late_enable(info);
		return;
	}

	/* Select PHY MUX */
	if (info->dual_phy) {
		u32 physel;

		physel = readl(regs_base + EXYNOS_USBCON_DUALPHYSEL);
		if (info->used_phy_port == 0) {
			physel &= ~DUALPHYSEL_PHYSEL_CTRL;
			physel &= ~DUALPHYSEL_PHYSEL_SSPHY;
			physel &= ~DUALPHYSEL_PHYSEL_PIPECLK;
			physel &= ~DUALPHYSEL_PHYSEL_PIPERST;
		} else {
			physel |= DUALPHYSEL_PHYSEL_CTRL;
			physel |= DUALPHYSEL_PHYSEL_SSPHY;
			physel |= DUALPHYSEL_PHYSEL_PIPECLK;
			physel |= DUALPHYSEL_PHYSEL_PIPERST;
		}
		writel(physel, regs_base + EXYNOS_USBCON_DUALPHYSEL);
	}

	/* Follow setting sequence for USB Link */
	/* 1. Set VBUS Valid and DP-Pull up control
	 * by VBUS pad usage */
	reg = readl(regs_base + EXYNOS_USBCON_UTMI);
	reg_hsp = readl(regs_base + EXYNOS_USBCON_HSP);
	if (info->not_used_vbus_pad) {
		link_vbus_filter_en(info, false);
		reg |= UTMI_FORCE_BVALID;
		reg |= UTMI_FORCE_VBUSVALID;
		reg_hsp |= HSP_VBUSVLDEXTSEL;
		reg_hsp |= HSP_VBUSVLDEXT;

	} else {
		link_vbus_filter_en(info, true);
		reg &= ~UTMI_FORCE_BVALID;
		reg &= ~UTMI_FORCE_VBUSVALID;
		reg_hsp &= ~HSP_VBUSVLDEXTSEL;
		reg_hsp &= ~HSP_VBUSVLDEXT;
	}
	writel(reg, regs_base + EXYNOS_USBCON_UTMI);
	writel(reg_hsp, regs_base + EXYNOS_USBCON_HSP);

	/* 2. OVC io usage */
	reg = readl(regs_base + EXYNOS_USBCON_LINK_PORT);
	if (info->use_io_for_ovc) {
		reg &= ~LINKPORT_HUB_PORT_SEL_OCD_U3;
		reg &= ~LINKPORT_HUB_PORT_SEL_OCD_U2;
	} else {
		reg |= LINKPORT_HUB_PORT_SEL_OCD_U3;
		reg |= LINKPORT_HUB_PORT_SEL_OCD_U2;
	}
	writel(reg, regs_base + EXYNOS_USBCON_LINK_PORT);

	/* Enable ReWA */
	if (info->hs_rewa)
		phy_exynos_usb3p1_rewa_ready(info);
}

enum exynos_usbcon_cr {
	USBCON_CR_ADDR = 0,
	USBCON_CR_DATA = 1,
	USBCON_CR_READ = 18,
	USBCON_CR_WRITE = 19,
};

static u16 phy_exynos_usb_v3p1_cr_access(struct exynos_usbphy_info *info,
	enum exynos_usbcon_cr cr_bit, u16 data)
{
	void __iomem *base;

	u32 ssp_crctl0;
	u32 ssp_crctl1;
	u32 loop;
	u32 loop_cnt;

	if (info->used_phy_port != -1) {
		if (info->used_phy_port == 0)
			base = info->regs_base;
		else
			base = info->regs_base_2nd;
	} else
		base = info->regs_base;

	/*Clear CR port register*/
	ssp_crctl0 = readl(base + EXYNOS_USBCON_SSP_CRCTL0);
	ssp_crctl0 &= ~(0xf);
	ssp_crctl0 &= ~(0xffff << 16);
	writel(ssp_crctl0, base + EXYNOS_USBCON_SSP_CRCTL0);

	/*Set Data for cr port*/
	ssp_crctl0 &= ~SSP_CCTRL0_CR_DATA_IN_MASK;
	ssp_crctl0 |= SSP_CCTRL0_CR_DATA_IN(data);
	writel(ssp_crctl0, base + EXYNOS_USBCON_SSP_CRCTL0);

	if (cr_bit == USBCON_CR_ADDR)
		loop = 1;
	else
		loop = 2;

	for (loop_cnt = 0; loop_cnt < loop; loop_cnt++) {
		u32 trigger_bit = 0;
		u32 handshake_cnt = 2;
		/* Trigger cr port */
		if (cr_bit == USBCON_CR_ADDR)
			trigger_bit = SSP_CRCTRL0_CR_CAP_ADDR;
		else {
			if (loop_cnt == 0)
				trigger_bit = SSP_CRCTRL0_CR_CAP_DATA;
			else {
				if (cr_bit == USBCON_CR_READ)
					trigger_bit = SSP_CRCTRL0_CR_READ;
				else
					trigger_bit = SSP_CRCTRL0_CR_WRITE;
			}
		}
		/* Handshake Procedure */
		do {
			u32 usec = 100;
			if (handshake_cnt == 2)
				ssp_crctl0 |= trigger_bit;
			else
				ssp_crctl0 &= ~trigger_bit;
			writel(ssp_crctl0, base + EXYNOS_USBCON_SSP_CRCTL0);

			/* Handshake */
			do {
				ssp_crctl1 = readl(base + EXYNOS_USBCON_SSP_CRCTL1);
				if ((handshake_cnt == 2) && (ssp_crctl1 & SSP_CRCTL1_CR_ACK))
					break;
				else if ((handshake_cnt == 1) && !(ssp_crctl1 & SSP_CRCTL1_CR_ACK))
					break;

				udelay(1);
			} while (usec-- > 0);

			if (!usec)
				pr_err("CRPORT handshake timeout1 (0x%08x)\n", ssp_crctl0);

			udelay(5);
			handshake_cnt--;
		} while (handshake_cnt != 0);
		udelay(50);

	}
	return (u16) ((ssp_crctl1 & SSP_CRCTL1_CR_DATA_OUT_MASK) >> 16);
}

void phy_exynos_usb_v3p1_cal_cr_write(struct exynos_usbphy_info *info, u16 addr, u16 data)
{
	phy_exynos_usb_v3p1_cr_access(info, USBCON_CR_ADDR, addr);
	phy_exynos_usb_v3p1_cr_access(info, USBCON_CR_WRITE, data);
}

u16 phy_exynos_usb_v3p1_cal_cr_read(struct exynos_usbphy_info *info, u16 addr)
{
	phy_exynos_usb_v3p1_cr_access(info, USBCON_CR_ADDR, addr);
	return phy_exynos_usb_v3p1_cr_access(info, USBCON_CR_WRITE, 0);
}

void phy_exynos_usb_v3p1_cal_usb3phy_tune_fix_rxeq(struct exynos_usbphy_info *info)
{
	u16 reg;
	struct exynos_usbphy_ss_tune *tune = info->ss_tune;

	if (!tune)
		return;
	reg = phy_exynos_usb_v3p1_cal_cr_read(info, 0x1006);
	reg &= ~(1 << 6);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x1006, reg);

	udelay(10);

	reg |= (1 << 7);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x1006, reg);

	udelay(10);

	reg &= ~(0x7 << 0x8);
	reg |= (tune->fix_rxeq_value << 8);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x1006, reg);

	udelay(10);

	reg |= (1 << 11);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x1006, reg);

	pr_info("Reg RX_OVRD_IN_HI : 0x%x\n",
		phy_exynos_usb_v3p1_cal_cr_read(info, 0x1006));
	pr_info("Reg RX_CDR_CDR_FSM_DEBUG : 0x%x\n",
		phy_exynos_usb_v3p1_cal_cr_read(info, 0x101c));
}

static void set_ss_tx_impedance(struct exynos_usbphy_info *info)
{
	u16 rtune_debug, tx_ovrd_in_hi;
	u8 tx_imp;

	/* obtain calibration code for 45Ohm impedance */
	rtune_debug = phy_exynos_usb_v3p1_cal_cr_read(info, 0x3);
	/* set SUP.DIG.RTUNE_DEBUG.TYPE = 2 */
	rtune_debug &= ~(0x3 << 3);
	rtune_debug |= (0x2 << 3);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x3, rtune_debug);

	/* read SUP.DIG.RTUNE_STAT (0x0004[9:0]) */
	tx_imp = phy_exynos_usb_v3p1_cal_cr_read(info, 0x4);
	/* current_tx_cal_code[9:0] = SUP.DIG.RTUNE_STAT (0x0004[9:0]) */
	tx_imp += 8;
	/* tx_cal_code[9:0] = current_tx_cal_code[9:0] + 8(decimal)
	 * NOTE, max value is 63;
	 * i.e. if tx_cal_code[9:0] > 63, tx_cal_code[9:0]==63;
	 */
	if (tx_imp > 63)
		tx_imp = 63;

	/* set LANEX_DIG_TX_OVRD_IN_HI.TX_RESET_OVRD = 1 */
	tx_ovrd_in_hi = phy_exynos_usb_v3p1_cal_cr_read(info, 0x1001);
	tx_ovrd_in_hi |= (1 << 7);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x1001, tx_ovrd_in_hi);

	/* SUP.DIG.RTUNE_DEBUG.MAN_TUNE = 0 */
	rtune_debug &= ~(1 << 1);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x3, rtune_debug);

	/* set SUP.DIG.RTUNE_DEBUG.VALUE = tx_cal_code[9:0] */
	rtune_debug &= ~(0x1ff << 5);
	rtune_debug |= (tx_imp << 5);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x3, rtune_debug);

	/* set SUP.DIG.RTUNE_DEBUG.SET_VAL = 1 */
	rtune_debug |= (1 << 2);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x3, rtune_debug);

	/* set SUP.DIG.RTUNE_DEBUG.SET_VAL = 0 */
	rtune_debug &= ~(1 << 2);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x3, rtune_debug);

	/* set LANEX_DIG_TX_OVRD_IN_HI.TX_RESET = 1 */
	tx_ovrd_in_hi |= (1 << 6);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x1001, tx_ovrd_in_hi);

	/* set LANEX_DIG_TX_OVRD_IN_HI.TX_RESET = 0 */
	tx_ovrd_in_hi &= ~(1 << 6);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x1001, tx_ovrd_in_hi);
}

void phy_exynos_usb_v3p1_cal_usb3phy_tune_adaptive_eq(
	struct exynos_usbphy_info *info, u8 eq_fix)
{
	u16 reg;

	reg = phy_exynos_usb_v3p1_cal_cr_read(info, 0x1006);
	if (eq_fix) {
		reg |= (1 << 6);
		reg &= ~(1 << 7);
	} else {
		reg &= ~(1 << 6);
		reg |= (1 << 7);
	}
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x1006, reg);
}

void phy_exynos_usb_v3p1_usb3phy_tune_chg_rxeq(
	struct exynos_usbphy_info *info, u8 eq_val)
{
	u16 reg;

	reg = phy_exynos_usb_v3p1_cal_cr_read(info, 0x1006);
	reg &= ~(0x7 << 0x8);
	reg |= ((eq_val & 0x7) << 8);
	reg |= (1 << 11);
	phy_exynos_usb_v3p1_cal_cr_write(info, 0x1006, reg);
}

void phy_exynos_usb_v3p1_late_enable(struct exynos_usbphy_info *info)
{
	u32 version = info->version;

	if (version > EXYNOS_USBCON_VER_05_0_0) {
		struct exynos_usbphy_ss_tune *tune = info->ss_tune;

		/*Set RXDET_MEAS_TIME[11:4] each reference clock*/
		phy_exynos_usb_v3p1_cal_cr_write(info, 0x1010, 0x80);

		if (tune) {
			phy_exynos_usb_v3p1_cal_cr_write(info, 0x1002, 0x4000 |
							 (tune->tx_deemphasis_3p5db << 7) |
							 tune->tx_swing_full);
			if (tune->rx_decode_mode)
				phy_exynos_usb_v3p1_cal_cr_write(info, 0x1026, 0x1);
			if (tune->set_crport_level_en) {
				/* Enable override los_bias, los_level and
				 * tx_vboost_lvl, Set los_bias to 0x5 and
				 * los_level to 0x9
				 */
				phy_exynos_usb_v3p1_cal_cr_write(info, 0x15, 0xA409);
				/* Set TX_VBOOST_LEVLE to tune->tx_boost_level */
				phy_exynos_usb_v3p1_cal_cr_write(info, 0x12, tune->tx_boost_level<<13);
			}
			/* to set the charge pump proportional current */
			if (tune->set_crport_mpll_charge_pump)
				phy_exynos_usb_v3p1_cal_cr_write(info, 0x30, 0xC0);

			if (tune->enable_fixed_rxeq_mode)
				phy_exynos_usb_v3p1_cal_usb3phy_tune_fix_rxeq(info);

			if (tune->decrease_ss_tx_imp)
				set_ss_tx_impedance(info);
		}
	}
}

void phy_exynos_usb_v3p1_disable(struct exynos_usbphy_info *info)
{
	u32 reg;
	void __iomem *regs_base = info->regs_base;

	/* set phy clock & control HS phy */
	reg = readl(regs_base + EXYNOS_USBCON_UTMI);
	reg |= UTMI_FORCE_SUSPEND;
	reg |= UTMI_FORCE_SLEEP;
	writel(reg, regs_base + EXYNOS_USBCON_UTMI);

	/* Disable PHY Power Mode */
	phy_power_en(info, 0);

	/* clear force q-channel */
	exynos_cal_usbphy_q_ch(regs_base, 0);
}

void phy_exynos_usb_v3p1_config_host_mode(struct exynos_usbphy_info *info)
{
	u32 reg;
	void __iomem *regs_base = info->regs_base;

	/* DP/DM Pull Down Control */
	reg = readl(regs_base + EXYNOS_USBCON_UTMI);
	reg |= UTMI_DP_PULLDOWN;
	reg |= UTMI_DM_PULLDOWN;
	reg &= ~UTMI_FORCE_BVALID;
	reg &= ~UTMI_FORCE_VBUSVALID;
	writel(reg, regs_base + EXYNOS_USBCON_UTMI);

	/* Disable Pull-up Register */
	reg = readl(regs_base + EXYNOS_USBCON_HSP);
	reg &= ~HSP_VBUSVLDEXTSEL;
	reg &= ~HSP_VBUSVLDEXT;
	writel(reg, regs_base + EXYNOS_USBCON_HSP);
}

void phy_exynos_usb_v3p1_enable_dp_pullup(struct exynos_usbphy_info *usbphy_info)
{
	void __iomem *regs_base = usbphy_info->regs_base;
	u32 phyutmi;

	phyutmi = readl(regs_base + EXYNOS_USBCON_HSP);
	phyutmi |= HSP_VBUSVLDEXT;
	writel(phyutmi, regs_base + EXYNOS_USBCON_HSP);
}

void phy_exynos_usb_v3p1_disable_dp_pullup(struct exynos_usbphy_info *usbphy_info)
{
	void __iomem *regs_base = usbphy_info->regs_base;
	u32 phyutmi;

	phyutmi = readl(regs_base + EXYNOS_USBCON_HSP);
	phyutmi &= ~HSP_VBUSVLDEXT;
	writel(phyutmi, regs_base + EXYNOS_USBCON_HSP);
}

void phy_exynos_usb_v3p1_tune_dev(struct exynos_usbphy_info *info)
{

}

void phy_exynos_usb_v3p1_tune_host(struct exynos_usbphy_info *info)
{

}

void phy_exynos_usb_v3p1_tune(struct exynos_usbphy_info *info)
{
	u32 reg, cnt;
	void __iomem *regs_base = info->regs_base;

	if (!info->tune_param)
		return;

	reg = readl(regs_base + EXYNOS_USBCON_HSP_TUNE);

	printk(KERN_ERR "usb: before HS Tune (0x%08x) \n", reg);
	cnt = 0;
	for (; info->tune_param[cnt].value != EXYNOS_USB_TUNE_LAST; cnt++) {
		char *para_name;
		int val;

		val = info->tune_param[cnt].value;
		if (val == -1)
			continue;
		para_name = info->tune_param[cnt].name;
		if (!strcmp(para_name, "compdis")) {
			reg &= ~HSP_TUNE_COMPDIS_MASK;
			reg |= HSP_TUNE_COMPDIS_SET(val);
		} else if (!strcmp(para_name, "otg")) {
			reg &= ~HSP_TUNE_OTG_MASK;
			reg |= HSP_TUNE_OTG_SET(val);
		} else if (!strcmp(para_name, "rx_sqrx")) {
			reg &= ~HSP_TUNE_SQRX_MASK;
			reg |= HSP_TUNE_SQRX_SET(val);
		} else if (!strcmp(para_name, "tx_fsls")) {
			reg &= ~HSP_TUNE_TXFSLS_MASK;
			reg |= HSP_TUNE_TXFSLS_SET(val);
		} else if (!strcmp(para_name, "tx_hsxv")) {
			reg &= ~HSP_TUNE_HSXV_MASK;
			reg |= HSP_TUNE_HSXV_SET(val);
		} else if (!strcmp(para_name, "tx_pre_emp")) {
			reg &= ~HSP_TUNE_TXPREEMPA_MASK;
			reg |= HSP_TUNE_TXPREEMPA_SET(val);
		} else if (!strcmp(para_name, "tx_pre_emp_plus")) {
			if (val)
				reg |= HSP_TUNE_TXPREEMPA_PLUS;
			else
				reg &= ~HSP_TUNE_TXPREEMPA_PLUS;
		} else if (!strcmp(para_name, "tx_res")) {
			reg &= ~HSP_TUNE_TXRES_MASK;
			reg |= HSP_TUNE_TXRES_SET(val);
		} else if (!strcmp(para_name, "tx_rise")) {
			reg &= ~HSP_TUNE_TXRISE_MASK;
			reg |= HSP_TUNE_TXRISE_SET(val);
		} else if (!strcmp(para_name, "tx_vref")) {
			reg &= ~HSP_TUNE_TXVREF_MASK;
			reg |= HSP_TUNE_TXVREF_SET(val);
		}
	}
	writel(reg, regs_base + EXYNOS_USBCON_HSP_TUNE);
	printk(KERN_ERR "usb: after HS Tune (0x%08x) \n", reg);
}

void phy_exynos_usb_v3p1_tune_each(struct exynos_usbphy_info *info,
	char *para_name, int val)
{
	u32 reg;
	void __iomem *regs_base = info->regs_base;

	if (val == -1)
		return;

	reg = readl(regs_base + EXYNOS_USBCON_HSP_TUNE);
	if (!strcmp(para_name, "compdis")) {
		reg &= ~HSP_TUNE_COMPDIS_MASK;
		reg |= HSP_TUNE_COMPDIS_SET(val);
	} else if (!strcmp(para_name, "otg")) {
		reg &= ~HSP_TUNE_OTG_MASK;
		reg |= HSP_TUNE_OTG_SET(val);
	} else if (!strcmp(para_name, "rx_sqrx")) {
		reg &= ~HSP_TUNE_SQRX_MASK;
		reg |= HSP_TUNE_SQRX_SET(val);
	} else if (!strcmp(para_name, "tx_fsls")) {
		reg &= ~HSP_TUNE_TXFSLS_MASK;
		reg |= HSP_TUNE_TXFSLS_SET(val);
	} else if (!strcmp(para_name, "tx_hsxv")) {
		reg &= ~HSP_TUNE_HSXV_MASK;
		reg |= HSP_TUNE_HSXV_SET(val);
	} else if (!strcmp(para_name, "tx_pre_emp")) {
		reg &= ~HSP_TUNE_TXPREEMPA_MASK;
		reg |= HSP_TUNE_TXPREEMPA_SET(val);
	} else if (!strcmp(para_name, "tx_pre_emp_plus")) {
		if (val)
			reg |= HSP_TUNE_TXPREEMPA_PLUS;
		else
			reg &= ~HSP_TUNE_TXPREEMPA_PLUS;
	} else if (!strcmp(para_name, "tx_res")) {
		reg &= ~HSP_TUNE_TXRES_MASK;
		reg |= HSP_TUNE_TXRES_SET(val);
	} else if (!strcmp(para_name, "tx_rise")) {
		reg &= ~HSP_TUNE_TXRISE_MASK;
		reg |= HSP_TUNE_TXRISE_SET(val);
	} else if (!strcmp(para_name, "tx_vref")) {
		reg &= ~HSP_TUNE_TXVREF_MASK;
		reg |= HSP_TUNE_TXVREF_SET(val);
	}
	writel(reg, regs_base + EXYNOS_USBCON_HSP_TUNE);
}

void phy_exynos_usb_v3p1_wr_tune_reg(struct exynos_usbphy_info *info, u32 val)
{
	void __iomem *regs_base = info->regs_base;

	writel(val, regs_base + EXYNOS_USBCON_HSP_TUNE);
}

void phy_exynos_usb_v3p1_rd_tune_reg(struct exynos_usbphy_info *info, u32 *val)
{
	void __iomem *regs_base = info->regs_base;

	if (!val)
		return;

	*val = readl(regs_base + EXYNOS_USBCON_HSP_TUNE);
}

void phy_exynos_usb3p1_rewa_ready(struct exynos_usbphy_info *info)
{
	u32 reg;
	void __iomem *regs_base = info->regs_base;

	/* Disable ReWA */
	reg = readl(regs_base + EXYNOS_USBCON_REWA_ENABLE);
	reg &= ~REWA_ENABLE_HS_REWA_EN;
	writel(reg, regs_base + EXYNOS_USBCON_REWA_ENABLE);

	/* Config ReWA Operation */
	reg = readl(regs_base + EXYNOS_USBCON_HSREWA_CTRL);
	/* Select line state check circuit
	 * 0 : FSVPLUS/FSMINUS
	 * 1 : LINE STATE
	 * */
	reg &= ~HSREWA_CTRL_DPDM_MON_SEL;
	/* Select Drive K circuit
	 * 0 : Auto Resume in the PHY
	 * 1 : BYPASS mode by ReWA
	 * */
	reg |= HSREWA_CTRL_DIG_BYPASS_CON_EN;
	writel(reg, regs_base + EXYNOS_USBCON_HSREWA_CTRL);

	/* Set Driver K Time */
	reg = 0x1;
	writel(reg, regs_base + EXYNOS_USBCON_HSREWA_HSTK);

	/* Set Timeout counter Driver K Time */
	reg = 0xff00;
	writel(reg, regs_base + EXYNOS_USBCON_HSREWA_REFTO);

	/* Disable All events source for abnormal event generation */
	reg = readl(regs_base + EXYNOS_USBCON_HSREWA_INT1_MASK);
	reg |= HSREWA_CTRL_HS_EVT_ERR_SUS |
			HSREWA_CTRL_HS_EVT_ERR_DEV_K |
			HSREWA_CTRL_HS_EVT_DISCON |
			HSREWA_CTRL_HS_EVT_BYPASS_DIS |
			HSREWA_CTRL_HS_EVT_RET_DIS |
			HSREWA_CTRL_HS_EVT_RET_EN;
	writel(reg, regs_base + EXYNOS_USBCON_HSREWA_INT1_MASK);
}

int phy_exynos_usb3p1_rewa_enable(struct exynos_usbphy_info *info)
{
	int cnt;
	u32 reg;
	void __iomem *regs_base = info->regs_base;

	/* Clear the system valid flag */
	reg = readl(regs_base + EXYNOS_USBCON_HSREWA_CTRL);
	reg &= ~HSREWA_CTRL_HS_SYS_VALID;
	writel(reg, regs_base + EXYNOS_USBCON_HSREWA_CTRL);

	/* Enable ReWA */
	reg = readl(regs_base + EXYNOS_USBCON_REWA_ENABLE);
	reg |= REWA_ENABLE_HS_REWA_EN;
	writel(reg, regs_base + EXYNOS_USBCON_REWA_ENABLE);

	/* Check Status : Wait ReWA Status is retention enabled */
	for (cnt = 15; cnt != 0; cnt--) {

		reg = readl(regs_base + EXYNOS_USBCON_HSREWA_INT1_EVT);

		/* non suspend status*/
		if (reg & HSREWA_CTRL_HS_EVT_ERR_SUS)
			return HS_REWA_EN_STS_NOT_SUSPEND;
		/* Disconnect Status */
		if (reg & HSREWA_CTRL_HS_EVT_DISCON)
			return HS_REWA_EN_STS_DISCONNECT;
		/* Success ReWA Enable */
		if (reg & HSREWA_CTRL_HS_EVT_RET_EN)
			break;
		udelay(30);
	}

	if (!cnt) {
		/* Set the system valid flag */
		reg = readl(regs_base + EXYNOS_USBCON_HSREWA_CTRL);
		reg |= HSREWA_CTRL_HS_SYS_VALID;
		writel(reg, regs_base + EXYNOS_USBCON_HSREWA_CTRL);

		/*  Disable ReWA */
		reg = readl(regs_base + EXYNOS_USBCON_REWA_ENABLE);
		reg &= ~REWA_ENABLE_HS_REWA_EN;
		writel(reg, regs_base + EXYNOS_USBCON_REWA_ENABLE);

		udelay(90);

		return HS_REWA_EN_STS_NOT_SUSPEND;
	}

	/* Set the INT1 for detect K and Disconnect */
	reg = readl(regs_base + EXYNOS_USBCON_HSREWA_INT1_MASK);
	reg &= ~HSREWA_CTRL_HS_EVT_DISCON &
			~HSREWA_CTRL_HS_EVT_ERR_DEV_K;
	reg |= HSREWA_CTRL_HS_EVT_ERR_SUS |
			HSREWA_CTRL_HS_EVT_BYPASS_DIS |
			HSREWA_CTRL_HS_EVT_RET_DIS |
			HSREWA_CTRL_HS_EVT_RET_EN;
	writel(reg, regs_base + EXYNOS_USBCON_HSREWA_INT1_MASK);

	/*  Enable All interrupt source and disnable Wake-up event */
	reg = readl(regs_base + EXYNOS_USBCON_HSREWA_INTR);
	reg &= ~HSREWA_INTR_WAKEUP_REQ_MASK &
			~HSREWA_INTR_EVT_MASK &
			~HSREWA_INTR_WAKEUP_MASK &
			~HSREWA_INTR_TIMEOUT_MASK;
	writel(reg, regs_base + EXYNOS_USBCON_HSREWA_INTR);

	udelay(100);

	return HS_REWA_EN_STS_ENALBED;
}

int phy_exynos_usb3p1_rewa_req_sys_valid(struct exynos_usbphy_info *info)
{
	int cnt;
	u32 reg;
	void __iomem *regs_base = info->regs_base;

	/* Mask All Interrupt source for the INT1 */
	reg = readl(regs_base + EXYNOS_USBCON_HSREWA_INT1_MASK);
	reg &= ~HSREWA_CTRL_HS_EVT_DISCON &
			~HSREWA_CTRL_HS_EVT_ERR_DEV_K &
			~HSREWA_CTRL_HS_EVT_ERR_SUS &
			~HSREWA_CTRL_HS_EVT_BYPASS_DIS &
			~HSREWA_CTRL_HS_EVT_RET_DIS &
			~HSREWA_CTRL_HS_EVT_RET_EN;
	writel(reg, regs_base + EXYNOS_USBCON_HSREWA_INT1_MASK);

	/*  Enable All interrupt source and disnable Wake-up event */
	reg = readl(regs_base + EXYNOS_USBCON_HSREWA_INTR);
	reg |= HSREWA_INTR_WAKEUP_REQ_MASK;
	reg |= HSREWA_INTR_TIMEOUT_MASK;
	reg |= HSREWA_INTR_EVT_MASK;
	reg |= HSREWA_INTR_WAKEUP_MASK;
	writel(reg, regs_base + EXYNOS_USBCON_HSREWA_INTR);

	/* Set the system valid flag */
	reg = readl(regs_base + EXYNOS_USBCON_HSREWA_CTRL);
	reg |= HSREWA_CTRL_HS_SYS_VALID;
	writel(reg, regs_base + EXYNOS_USBCON_HSREWA_CTRL);

	for (cnt = 15; cnt != 0; cnt--) {

		reg = readl(regs_base + EXYNOS_USBCON_HSREWA_INT1_EVT);

		/* Disconnect Status */
		if (reg & HSREWA_CTRL_HS_EVT_DISCON)
			return HS_REWA_EN_STS_DISCONNECT;
		/* Success ReWA Enable */
		if (reg & HSREWA_CTRL_HS_EVT_RET_EN)
			break;
		udelay(30);
	}

	return HS_REWA_EN_STS_DISABLED;
}

int phy_exynos_usb3p1_rewa_disable(struct exynos_usbphy_info *info)
{
	int cnt;
	u32 reg;
	void __iomem *regs_base = info->regs_base;

	/* Set Link ready to notify ReWA */
	reg = readl(regs_base + EXYNOS_USBCON_HSREWA_CTRL);
	reg |= HSREWA_CTRL_HS_LINK_READY;
	writel(reg, regs_base + EXYNOS_USBCON_HSREWA_CTRL);
	/* Wait Bypass Disable */
	for (cnt = 15; cnt != 0; cnt--) {
		reg = readl(regs_base + EXYNOS_USBCON_HSREWA_INT1_EVT);
		/* Success ReWA Enable */
		if (reg & HSREWA_CTRL_HS_EVT_BYPASS_DIS)
			break;
		udelay(30);
	}
	if (!cnt)
		return -1;
	/* Wait ReWA Done */
	for (cnt = 15; cnt != 0; cnt--) {

		reg = readl(regs_base + EXYNOS_USBCON_HSREWA_CTRL);

		/* Success ReWA Enable */
		if (reg & HSREWA_CTRL_HS_REWA_DONE)
			break;
		udelay(30);
	}
	if (!cnt)
		return -1;
	/*  Disable ReWA */
	reg = readl(regs_base + EXYNOS_USBCON_REWA_ENABLE);
	reg &= ~REWA_ENABLE_HS_REWA_EN;
	writel(reg, regs_base + EXYNOS_USBCON_REWA_ENABLE);

	return 0;
}

int phy_exynos_usb3p1_rewa_cancel(struct exynos_usbphy_info *info)
{
	int ret;
	u32 reg;
	void __iomem *regs_base = info->regs_base;

	/* Check ReWA Already diabled
	 * If ReWA was disabled states, disabled sequence is already done */
	reg = readl(regs_base + EXYNOS_USBCON_REWA_ENABLE);
	if (!(reg & REWA_ENABLE_HS_REWA_EN))
		return 0;

	ret = phy_exynos_usb3p1_rewa_req_sys_valid(info);

	/*  Disable ReWA */
	reg = readl(regs_base + EXYNOS_USBCON_REWA_ENABLE);
	reg &= ~REWA_ENABLE_HS_REWA_EN;
	writel(reg, regs_base + EXYNOS_USBCON_REWA_ENABLE);

	udelay(100);

	return ret;
}
