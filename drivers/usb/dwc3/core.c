/**
 * core.c - DesignWare USB3 DRD Controller Core file
 *
 * Copyright (C) 2010-2011 Texas Instruments Incorporated - http://www.ti.com
 *
 * Authors: Felipe Balbi <balbi@ti.com>,
 *	    Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/acpi.h>
#include <linux/pinctrl/consumer.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/of.h>
#include <linux/usb/otg.h>

#include "platform_data.h"
#include "core.h"
#include "otg.h"
#include "gadget.h"
#include "io.h"

#include "debug.h"
/* for BC1.2 spec */
int dwc3_set_vbus_current(int state)
{
	struct power_supply *psy;
	union power_supply_propval pval = {0};

	psy = power_supply_get_by_name("battery");
	if(!psy) {
		pr_err("%s: fail to get battery power_supply\n", __func__);
		return -1;
	}

	pval.intval = state; 
	power_supply_set_property(psy, POWER_SUPPLY_EXT_PROP_USB_CONFIGURE, &pval);
	power_supply_put(psy);
	return 0;
}

static void dwc3_exynos_set_vbus_current_work(struct work_struct *w)
{
	struct dwc3 *dwc = container_of(w, struct dwc3, set_vbus_current_work);

	dwc3_set_vbus_current(dwc->vbus_curernt);
}
/* -------------------------------------------------------------------------- */

void dwc3_set_mode(struct dwc3 *dwc, u32 mode)
{
	u32 reg;

	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg &= ~(DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_OTG));
	reg |= DWC3_GCTL_PRTCAPDIR(mode);
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);
}

void dwc3_core_config(struct dwc3 *dwc)
{
	u32 reg;

	/* AHB bus configuration */
	reg = dwc3_readl(dwc->regs, DWC3_GSBUSCFG0);
	reg |= (DWC3_GSBUSCFG0_INCRBRSTEN | DWC3_GSBUSCFG0_INCR16BRSTEN);
	/**
	 * AXI Bus' cache type configuration for DMA transfer.
	 * By below setting, cache type was set to Cacheable/Modifiable.
	 * From DWC USB3.0 Link version 2.20A, this cache type could be set.
	 */
	if (dwc->revision >= DWC3_REVISION_220A)
		reg |= (DWC3_GSBUSCFG0_DESWRREQINFO |
			DWC3_GSBUSCFG0_DATWRREQINFO |
			DWC3_GSBUSCFG0_DESRDREQINFO |
			DWC3_GSBUSCFG0_DATRDREQINFO);
	dwc3_writel(dwc->regs, DWC3_GSBUSCFG0, reg);

	reg = dwc3_readl(dwc->regs, DWC3_GSBUSCFG1);
	reg |= (DWC3_GSBUSCFG1_BREQLIMIT(3));
	dwc3_writel(dwc->regs, DWC3_GSBUSCFG1, reg);

	/*
	 * WORKAROUND:
	 * For ss bulk-in data packet, when the host detects
	 * a DPP error or the internal buffer becomes full,
	 * it retries with an ACK TP Retry=1. Under the following
	 * conditions, the Retry=1 is falsely carried over to the next
	 * DWC3_GUCTL_USBHSTINAUTORETRYEN should be set to a one
	 * regardless of revision
	 * - There is only single active asynchronous SS EP at the time.
	 * - The active asynchronous EP is a Bulk IN EP.
	 * - The burst with the correctly Retry=1 ACK TP and
	 *   the next burst belong to the same transfer.
	 */
	reg = dwc3_readl(dwc->regs, DWC3_GUCTL);
	reg |= (DWC3_GUCTL_USBHSTINAUTORETRYEN);
	if (dwc->adj_sof_accuracy) {
		reg &= ~DWC3_GUCTL_REFCLKPER_MASK;
		reg |= DWC3_GUCTL_REFCLKPER(0x29);
	}
	if (dwc->sparse_transfer_control)
		reg |= DWC3_GUCTL_SPRSCTRLTRANSEN;
	dwc3_writel(dwc->regs, DWC3_GUCTL, reg);
	if (dwc->revision >= DWC3_REVISION_190A &&
		dwc->revision <= DWC3_REVISION_210A) {
		reg = dwc3_readl(dwc->regs, DWC3_GRXTHRCFG);
		reg &= ~(DWC3_GRXTHRCFG_USBRXPKTCNT_MASK |
			DWC3_GRXTHRCFG_USBMAXRXBURSTSIZE_MASK);
		reg |= (DWC3_GRXTHRCFG_USBRXPKTCNTSEL |
			DWC3_GRXTHRCFG_USBRXPKTCNT(3) |
			DWC3_GRXTHRCFG_USBMAXRXBURSTSIZE(3));
		dwc3_writel(dwc->regs, DWC3_GRXTHRCFG, reg);
	}

	/*
	 * WORKAROUND: DWC3 revisions 2.10a and earlier have a bug
	 * The delay of the entry to a low power state such that
	 * for applications where the link stays in a non-U0 state
	 * for a short duration(< 1 microsecond),
	 * the local PHY does not enter the low power state prior
	 * to receiving a potential LFPS wakeup.
	 * This causes the PHY CDR (Clock and Data Recovery) operation
	 * to be unstable for some Synopsys PHYs.
	 * The proposal now is to change the default and the recommended value
	 * for GUSB3PIPECTL[21:19] in the RTL from 3'b100 to a minimum of 3'b001
	 */
	if (dwc->revision <= DWC3_REVISION_210A) {
		reg = dwc3_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));
		reg &= ~(DWC3_GUSB3PIPECTL_DEP1P2P3_MASK);
		reg |= (DWC3_GUSB3PIPECTL_DEP1P2P3_EN);
		dwc3_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);
	}

	if (dwc->revision >= DWC3_REVISION_250A) {
		reg = dwc3_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));
		reg |= DWC3_GUSB3PIPECTL_DIS_RXDETP3;
		dwc3_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);
	}

	/*
	 * WORKAROUND: DWC3 revisions 2.10a and earlier have a bug
	 * Race Condition in PORTSC Write Followed by Read
	 * If the software quickly does a read to the PORTSC,
	 * some fields (port status change related fields
	 * like OCC, etc.) may not have correct value
	 * due to the current way of handling these bits.
	 * After clearing the status register (for example, OCC) bit
	 * by writing PORTSC tregister, software can insert some delay
	 * (for example, 5 mac2_clk -> UTMI clock = 60 MHz ->
	 * (16.66 ns x 5 = 84ns)) before reading the PORTSC to check status.
	 */
	if (dwc->revision <= DWC3_REVISION_210A) {
		reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));
		reg |= (DWC3_GUSB2PHYCFG_PHYIF);
		dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);
	}
}

/**
 * dwc3_core_soft_reset - Issues core soft reset and PHY reset
 * @dwc: pointer to our context structure
 */
static int dwc3_core_soft_reset(struct dwc3 *dwc)
{
	u32 	reg;
	int 	retries = 1000;
	int 	ret;
	int 	try_cnt = 0;

	ret = phy_power_on(dwc->usb3_generic_phy);
	if (ret < 0)
	    return ret;

	ret = phy_power_on(dwc->usb2_generic_phy);
	if (ret < 0)
	    goto err_usb3phy_power;

	usb_phy_init(dwc->usb2_phy);
	usb_phy_init(dwc->usb3_phy);
	ret = phy_init(dwc->usb2_generic_phy);
	if (ret < 0)
		goto err_usb2phy_power;

	ret = phy_init(dwc->usb3_generic_phy);
	if (ret < 0)
		goto err_usb2phy_init;

	/*
	 * We're resetting only the device side because, if we're in host mode,
	 * XHCI driver will reset the host block. If dwc3 was configured for
	 * host-only mode, then we can return early.
	 */
	if (dwc->dr_mode == USB_DR_MODE_HOST)
		return 0;

reset_dctl:
	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg |= DWC3_DCTL_CSFTRST;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	retries = 1000;
	do {
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		if (!(reg & DWC3_DCTL_CSFTRST))
			goto reset_check;

		udelay(1);
	} while (--retries);

	ret = -ETIMEDOUT;
	goto err_usb3phy_init;

reset_check:
	if (!dwc->suspend_clk_freq)
		dwc->suspend_clk_freq = 50000000;

	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg &= ~DWC3_GCTL_PWRDNSCALE_MASK;
	reg |= DWC3_GCTL_PWRDNSCALE(dwc->suspend_clk_freq/(16*1000));
	reg &= ~0xc0;
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);

	/* check for CSR Timeout */
	retries = 50;
	do {
		dwc3_writel(dwc->regs, DWC3_GEVNTADRLO(0), DWC3_MAGIC_LO);
		reg = dwc3_readl(dwc->regs, DWC3_GEVNTADRLO(0));
		if (reg == DWC3_MAGIC_LO)
			return 0;
		udelay(1);
	} while(--retries); 
	dev_err(dwc->dev, "ERR!! GEVNTADRL0 r/w fail\n");

	reg = dwc3_readl(dwc->regs, DWC3_GSTS);
	if (reg & DWC3_GSTS_CSR_TIMEOUT)
		dev_err(dwc->dev, "%s: CSR Timeout !!\n", __func__);

	if (++try_cnt > MAX_RETRY_CNT) {
		dev_err(dwc->dev, "PHY Reset again !!\n");
		ret = -ETIMEDOUT;
	} else {
		dev_err(dwc->dev, "DCTL Reset again !!\n");
		goto reset_dctl;
	}	

err_usb3phy_init:
	phy_exit(dwc->usb3_generic_phy);

err_usb2phy_init:
	phy_exit(dwc->usb2_generic_phy);

err_usb2phy_power:
	phy_power_off(dwc->usb2_generic_phy);

err_usb3phy_power:
	phy_power_off(dwc->usb3_generic_phy);

	return ret;
}

/**
 * dwc3_soft_reset - Issue soft reset
 * @dwc: Pointer to our controller context structure
 */
int dwc3_soft_reset(struct dwc3 *dwc)
{
	unsigned long timeout;
	u32 reg;

	timeout = jiffies + msecs_to_jiffies(500);
	dwc3_writel(dwc->regs, DWC3_DCTL, DWC3_DCTL_CSFTRST);
	do {
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		if (!(reg & DWC3_DCTL_CSFTRST))
			break;

		if (time_after(jiffies, timeout)) {
			dev_err(dwc->dev, "Reset Timed Out\n");
			return -ETIMEDOUT;
		}

		cpu_relax();
	} while (true);

	return 0;
}

/*
 * dwc3_frame_length_adjustment - Adjusts frame length if required
 * @dwc3: Pointer to our controller context structure
 * @fladj: Value of GFLADJ_30MHZ to adjust frame length
 */
static void dwc3_frame_length_adjustment(struct dwc3 *dwc, u32 fladj)
{
	u32 reg;
	u32 dft;

	if (dwc->revision < DWC3_REVISION_250A)
		return;

	if (fladj == 0)
		return;

	reg = dwc3_readl(dwc->regs, DWC3_GFLADJ);
	dft = reg & DWC3_GFLADJ_30MHZ_MASK;
	if (!dev_WARN_ONCE(dwc->dev, dft == fladj,
	    "request value same as default, ignoring\n")) {
		reg &= ~DWC3_GFLADJ_30MHZ_MASK;
		reg |= DWC3_GFLADJ_30MHZ_SDBND_SEL | fladj;
		dwc3_writel(dwc->regs, DWC3_GFLADJ, reg);
	}
}

/**
 * dwc3_free_one_event_buffer - Frees one event buffer
 * @dwc: Pointer to our controller context structure
 * @evt: Pointer to event buffer to be freed
 */
static void dwc3_free_one_event_buffer(struct dwc3 *dwc,
		struct dwc3_event_buffer *evt)
{
	dma_free_coherent(dwc->dev, evt->length, evt->buf, evt->dma);
}

/**
 * dwc3_alloc_one_event_buffer - Allocates one event buffer structure
 * @dwc: Pointer to our controller context structure
 * @length: size of the event buffer
 *
 * Returns a pointer to the allocated event buffer structure on success
 * otherwise ERR_PTR(errno).
 */
static struct dwc3_event_buffer *dwc3_alloc_one_event_buffer(struct dwc3 *dwc,
		unsigned length)
{
	struct dwc3_event_buffer	*evt;

	evt = devm_kzalloc(dwc->dev, sizeof(*evt), GFP_KERNEL);
	if (!evt)
		return ERR_PTR(-ENOMEM);

	evt->dwc	= dwc;
	evt->length	= length;
	evt->buf	= dma_alloc_coherent(dwc->dev, length,
			&evt->dma, GFP_KERNEL);
	if (!evt->buf)
		return ERR_PTR(-ENOMEM);

	return evt;
}

/**
 * dwc3_free_event_buffers - frees all allocated event buffers
 * @dwc: Pointer to our controller context structure
 */
static void dwc3_free_event_buffers(struct dwc3 *dwc)
{
	struct dwc3_event_buffer	*evt;
	int i;

	for (i = 0; i < dwc->num_event_buffers; i++) {
		evt = dwc->ev_buffs[i];
		if (evt)
			dwc3_free_one_event_buffer(dwc, evt);
	}
}

/**
 * dwc3_alloc_event_buffers - Allocates @num event buffers of size @length
 * @dwc: pointer to our controller context structure
 * @length: size of event buffer
 *
 * Returns 0 on success otherwise negative errno. In the error case, dwc
 * may contain some buffers allocated but not all which were requested.
 */
static int dwc3_alloc_event_buffers(struct dwc3 *dwc, unsigned length)
{
	int			num;
	int			i;

	num = DWC3_NUM_INT(dwc->hwparams.hwparams1);
	dwc->num_event_buffers = num;

	dwc->ev_buffs = devm_kzalloc(dwc->dev, sizeof(*dwc->ev_buffs) * num,
			GFP_KERNEL);
	if (!dwc->ev_buffs)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		struct dwc3_event_buffer	*evt;

		evt = dwc3_alloc_one_event_buffer(dwc, length);
		if (IS_ERR(evt)) {
			dev_err(dwc->dev, "can't allocate event buffer\n");
			return PTR_ERR(evt);
		}
		dwc->ev_buffs[i] = evt;
	}

	return 0;
}

/**
 * dwc3_event_buffers_setup - setup our allocated event buffers
 * @dwc: pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int dwc3_event_buffers_setup(struct dwc3 *dwc)
{
	struct dwc3_event_buffer	*evt;
	int				n;
	u32				reg1, reg2;

	for (n = 0; n < dwc->num_event_buffers; n++) {
		evt = dwc->ev_buffs[n];
		dev_info(dwc->dev, "Event buf %p dma %08llx length %d\n",
				evt->buf, (unsigned long long) evt->dma,
				evt->length);

		evt->lpos = 0;

		dwc3_writel(dwc->regs, DWC3_GEVNTADRLO(n),
				lower_32_bits(evt->dma));
		dwc3_writel(dwc->regs, DWC3_GEVNTADRHI(n),
				upper_32_bits(evt->dma));
		dwc3_writel(dwc->regs, DWC3_GEVNTSIZ(n),
				DWC3_GEVNTSIZ_SIZE(evt->length));
		dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(n), 0);

		reg1 = dwc3_readl(dwc->regs, DWC3_GEVNTADRLO(n));
		reg2 = dwc3_readl(dwc->regs, DWC3_GEVNTADRHI(n));
		dev_info(dwc->dev, "Event buf reg read 0x%08x %08x\n",
				reg2, reg1);
	}

	return 0;
}

void dwc3_event_buffers_cleanup(struct dwc3 *dwc)
{
	struct dwc3_event_buffer	*evt;
	int				n;

	for (n = 0; n < dwc->num_event_buffers; n++) {
		evt = dwc->ev_buffs[n];

		evt->lpos = 0;

		dwc3_writel(dwc->regs, DWC3_GEVNTSIZ(n), DWC3_GEVNTSIZ_INTMASK
				| DWC3_GEVNTSIZ_SIZE(0));
		dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(n), 0);
	}
}

static int dwc3_alloc_scratch_buffers(struct dwc3 *dwc)
{
	if (!dwc->has_hibernation)
		return 0;

	if (!dwc->nr_scratch)
		return 0;

	dwc->scratchbuf = kmalloc_array(dwc->nr_scratch,
			DWC3_SCRATCHBUF_SIZE, GFP_KERNEL);
	if (!dwc->scratchbuf)
		return -ENOMEM;

	return 0;
}

static int dwc3_setup_scratch_buffers(struct dwc3 *dwc)
{
	dma_addr_t scratch_addr;
	u32 param;
	int ret;

	if (!dwc->has_hibernation)
		return 0;

	if (!dwc->nr_scratch)
		return 0;

	 /* should never fall here */
	if (!WARN_ON(dwc->scratchbuf))
		return 0;

	scratch_addr = dma_map_single(dwc->dev, dwc->scratchbuf,
			dwc->nr_scratch * DWC3_SCRATCHBUF_SIZE,
			DMA_BIDIRECTIONAL);
	if (dma_mapping_error(dwc->dev, scratch_addr)) {
		dev_err(dwc->dev, "failed to map scratch buffer\n");
		ret = -EFAULT;
		goto err0;
	}

	dwc->scratch_addr = scratch_addr;
	dev_info(dwc->dev, "Scratch buf %p dma %08llx\n",
		dwc->scratchbuf,
		(unsigned long long) dwc->scratch_addr);

	param = lower_32_bits(scratch_addr);

	ret = dwc3_send_gadget_generic_command(dwc,
			DWC3_DGCMD_SET_SCRATCHPAD_ADDR_LO, param);
	if (ret < 0)
		goto err1;

	param = upper_32_bits(scratch_addr);

	ret = dwc3_send_gadget_generic_command(dwc,
			DWC3_DGCMD_SET_SCRATCHPAD_ADDR_HI, param);
	if (ret < 0)
		goto err1;

	return 0;

err1:
	dma_unmap_single(dwc->dev, dwc->scratch_addr, dwc->nr_scratch *
			DWC3_SCRATCHBUF_SIZE, DMA_BIDIRECTIONAL);

err0:
	return ret;
}

static void dwc3_free_scratch_buffers(struct dwc3 *dwc)
{
	if (!dwc->has_hibernation)
		return;

	if (!dwc->nr_scratch)
		return;

	 /* should never fall here */
	if (!WARN_ON(dwc->scratchbuf))
		return;

	dma_unmap_single(dwc->dev, dwc->scratch_addr, dwc->nr_scratch *
			DWC3_SCRATCHBUF_SIZE, DMA_BIDIRECTIONAL);
	kfree(dwc->scratchbuf);
}

void dwc3_link_status_check(struct dwc3 *dwc)
{
	u32 reg;

	reg = dwc3_readl(dwc->regs, DWC3_GUID);
	dev_info(dwc->dev, "%s: GUID 0x%08x\n", __func__, reg);
	reg = dwc3_readl(dwc->regs, DWC3_GSTS);
	dev_info(dwc->dev, "%s: GSTS 0x%08x\n", __func__, reg);
	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	dev_info(dwc->dev, "%s: DCTL 0x%08x\n", __func__, reg);
	reg = dwc3_readl(dwc->regs, DWC3_DSTS);
	dev_info(dwc->dev, "%s: DSTS 0x%08x\n", __func__, reg);
}

static void dwc3_core_num_eps(struct dwc3 *dwc)
{
	struct dwc3_hwparams	*parms = &dwc->hwparams;

	dwc->num_in_eps = DWC3_NUM_IN_EPS(parms);
	dwc->num_out_eps = DWC3_NUM_EPS(parms) - dwc->num_in_eps;

	dwc3_trace(trace_dwc3_core, "found %d IN and %d OUT endpoints",
			dwc->num_in_eps, dwc->num_out_eps);
}

static void dwc3_cache_hwparams(struct dwc3 *dwc)
{
	struct dwc3_hwparams	*parms = &dwc->hwparams;

	parms->hwparams0 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS0);
	parms->hwparams1 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS1);
	parms->hwparams2 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS2);
	parms->hwparams3 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS3);
	parms->hwparams4 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS4);
	parms->hwparams5 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS5);
	parms->hwparams6 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS6);
	parms->hwparams7 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS7);
	parms->hwparams8 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS8);
}

/**
 * dwc3_phy_setup - Configure USB PHY Interface of DWC3 Core
 * @dwc: Pointer to our controller context structure
 *
 * Returns 0 on success. The USB PHY interfaces are configured but not
 * initialized. The PHY interfaces and the PHYs get initialized together with
 * the core in dwc3_core_init.
 */
int dwc3_phy_setup(struct dwc3 *dwc)
{
	u32 reg;
	int ret;

	reg = dwc3_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));

	/*
	 * Above 1.94a, it is recommended to set DWC3_GUSB3PIPECTL_SUSPHY
	 * to '0' during coreConsultant configuration. So default value
	 * will be '0' when the core is reset. Application needs to set it
	 * to '1' after the core initialization is completed.
	 */
	if (dwc->revision > DWC3_REVISION_194A)
		reg |= DWC3_GUSB3PIPECTL_SUSPHY;

	if (dwc->u2ss_inp3_quirk)
		reg |= DWC3_GUSB3PIPECTL_U2SSINP3OK;

	if (dwc->req_p1p2p3_quirk)
		reg |= DWC3_GUSB3PIPECTL_REQP1P2P3;

	if (dwc->del_p1p2p3_quirk)
		reg |= DWC3_GUSB3PIPECTL_DEP1P2P3_EN;

	if (dwc->del_phy_power_chg_quirk)
		reg |= DWC3_GUSB3PIPECTL_DEPOCHANGE;

	if (dwc->lfps_filter_quirk)
		reg |= DWC3_GUSB3PIPECTL_LFPSFILT;

	if (dwc->rx_detect_poll_quirk)
		reg |= DWC3_GUSB3PIPECTL_RX_DETOPOLL;

	if (dwc->tx_de_emphasis_quirk)
		reg |= DWC3_GUSB3PIPECTL_TX_DEEPH(dwc->tx_de_emphasis);

	if (dwc->dis_u3_susphy_quirk)
		reg &= ~DWC3_GUSB3PIPECTL_SUSPHY;

	dwc3_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);

	reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));

	/* Select the HS PHY interface */
	switch (DWC3_GHWPARAMS3_HSPHY_IFC(dwc->hwparams.hwparams3)) {
	case DWC3_GHWPARAMS3_HSPHY_IFC_UTMI_ULPI:
		if (dwc->hsphy_interface &&
				!strncmp(dwc->hsphy_interface, "utmi", 4)) {
			reg &= ~DWC3_GUSB2PHYCFG_ULPI_UTMI;
			break;
		} else if (dwc->hsphy_interface &&
				!strncmp(dwc->hsphy_interface, "ulpi", 4)) {
			reg |= DWC3_GUSB2PHYCFG_ULPI_UTMI;
			dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);
		} else {
			/* Relying on default value. */
			if (!(reg & DWC3_GUSB2PHYCFG_ULPI_UTMI))
				break;
		}
		/* FALLTHROUGH */
	case DWC3_GHWPARAMS3_HSPHY_IFC_ULPI:
		/* Making sure the interface and PHY are operational */
		ret = dwc3_soft_reset(dwc);
		if (ret)
			return ret;

		udelay(1);

		ret = dwc3_ulpi_init(dwc);
		if (ret)
			return ret;
		/* FALLTHROUGH */
	default:
		break;
	}

	/*
	 * Above 1.94a, it is recommended to set DWC3_GUSB2PHYCFG_SUSPHY to
	 * '0' during coreConsultant configuration. So default value will
	 * be '0' when the core is reset. Application needs to set it to
	 * '1' after the core initialization is completed.
	 */
	if (dwc->revision > DWC3_REVISION_194A)
		reg |= DWC3_GUSB2PHYCFG_SUSPHY;

	if (dwc->dis_u2_susphy_quirk)
		reg &= ~DWC3_GUSB2PHYCFG_SUSPHY;

	if (dwc->dis_enblslpm_quirk)
		reg &= ~DWC3_GUSB2PHYCFG_ENBLSLPM;

	if (dwc->adj_sof_accuracy)
		reg &= ~DWC3_GUSB2PHYCFG_U2_FREECLK_EXISTS;

	reg &= ~DWC3_GUSB2PHYCFG_ENBLSLPM;
	reg &= ~DWC3_GUSB2PHYCFG_SUSPHY;
	dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);

	return 0;
}

/**
 * dwc3_core_init - Low-level initialization of DWC3 Core
 * @dwc: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int dwc3_core_init(struct dwc3 *dwc)
{
	u32			hwparams4 = dwc->hwparams.hwparams4;
	u32			reg;
	struct dwc3_otg		*dotg = dwc->dotg;
	int			retries = 2;
	int			ret;

	reg = dwc3_readl(dwc->regs, DWC3_GSNPSID);
	/* This should read as U3 followed by revision number */
	if ((reg & DWC3_GSNPSID_MASK) == 0x55330000) {
		/* Detected DWC_usb3 IP */
		dwc->revision = reg;
	} else if ((reg & DWC3_GSNPSID_MASK) == 0x33310000) {
		/* Detected DWC_usb31 IP */
		dwc->revision = dwc3_readl(dwc->regs, DWC3_VER_NUMBER);
		dwc->revision |= DWC3_REVISION_IS_DWC31;
	} else {
		dev_err(dwc->dev, "this is not a DesignWare USB3 DRD Core\n");
		ret = -ENODEV;
		goto err0;
	}

	/*
	 * Write Linux Version Code to our GUID register so it's easy to figure
	 * out which kernel version a bug was found.
	 */
	dwc3_writel(dwc->regs, DWC3_GUID, LINUX_VERSION_CODE);

	/* Handle USB2.0-only core configuration */
	if (DWC3_GHWPARAMS3_SSPHY_IFC(dwc->hwparams.hwparams3) ==
			DWC3_GHWPARAMS3_SSPHY_IFC_DIS) {
		if (dwc->maximum_speed == USB_SPEED_SUPER)
			dwc->maximum_speed = USB_SPEED_HIGH;
	}

	/* Adjust SOF accuracy only for revisions >= 2.50a */
	if (dwc->revision < DWC3_REVISION_250A)
		dwc->adj_sof_accuracy = 0;

	/* issue device SoftReset too */
	ret = dwc3_soft_reset(dwc);
	if (ret)
		goto err0;

	while (retries--) {
		ret = dwc3_core_soft_reset(dwc);
		if ((ret == -ETIMEDOUT) && retries)
			continue;
		else if (!ret)
			break;
		else if (ret)
			goto err0;
	}
	
	if (dotg) {
		phy_tune(dwc->usb2_generic_phy, dotg->otg.state);
		phy_tune(dwc->usb3_generic_phy, dotg->otg.state);
	} else {
		phy_tune(dwc->usb2_generic_phy, 0);
		phy_tune(dwc->usb3_generic_phy, 0);
	}

	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg &= ~DWC3_GCTL_SCALEDOWN_MASK;

	switch (DWC3_GHWPARAMS1_EN_PWROPT(dwc->hwparams.hwparams1)) {
	case DWC3_GHWPARAMS1_EN_PWROPT_CLK:
		/**
		 * WORKAROUND: DWC3 revisions between 2.10a and 2.50a have an
		 * issue which would cause xHCI compliance tests to fail.
		 *
		 * Because of that we cannot enable clock gating on such
		 * configurations.
		 *
		 * Refers to:
		 *
		 * STAR#9000588375: Clock Gating, SOF Issues when ref_clk-Based
		 * SOF/ITP Mode Used
		 */
		if ((dwc->dr_mode == USB_DR_MODE_HOST ||
				dwc->dr_mode == USB_DR_MODE_OTG) &&
				(dwc->revision >= DWC3_REVISION_210A &&
				dwc->revision <= DWC3_REVISION_250A))
			reg |= DWC3_GCTL_DSBLCLKGTNG | DWC3_GCTL_SOFITPSYNC;
		else
			reg &= ~DWC3_GCTL_DSBLCLKGTNG;
		break;
	case DWC3_GHWPARAMS1_EN_PWROPT_HIB:
		/* enable hibernation here */
		dwc->nr_scratch = DWC3_GHWPARAMS4_HIBER_SCRATCHBUFS(hwparams4);

		/*
		 * REVISIT Enabling this bit so that host-mode hibernation
		 * will work. Device-mode hibernation is not yet implemented.
		 */
		reg |= DWC3_GCTL_GBLHIBERNATIONEN;
		break;
	default:
		dev_dbg(dwc->dev, "No power optimization available\n");
	}

	/* check if current dwc3 is on simulation board */
	if (dwc->hwparams.hwparams6 & DWC3_GHWPARAMS6_EN_FPGA) {
		dev_dbg(dwc->dev, "it is on FPGA board\n");
		dwc->is_fpga = true;
	}

	WARN_ONCE(dwc->disable_scramble_quirk && !dwc->is_fpga,
			"disable_scramble cannot be used on non-FPGA builds\n");

	if (dwc->disable_scramble_quirk && dwc->is_fpga)
		reg |= DWC3_GCTL_DISSCRAMBLE;
	else
		reg &= ~DWC3_GCTL_DISSCRAMBLE;

	if (dwc->u2exit_lfps_quirk)
		reg |= DWC3_GCTL_U2EXIT_LFPS;

	/*
	 * WORKAROUND: DWC3 revisions <1.90a have a bug
	 * where the device can fail to connect at SuperSpeed
	 * and falls back to high-speed mode which causes
	 * the device to enter a Connect/Disconnect loop
	 */
	if (dwc->revision < DWC3_REVISION_190A)
		reg |= DWC3_GCTL_U2RSTECN;

	if (dwc->adj_sof_accuracy) {
		reg &= ~DWC3_GCTL_SOFITPSYNC;
		reg |= DWC3_GCTL_DSBLCLKGTNG;
	}

	dwc3_core_num_eps(dwc);

	dwc3_writel(dwc->regs, DWC3_GCTL, reg);

	dev_info(dwc->dev, "%s: max speed:%d, hibernation:%d, nr_scratch:%d\n",
			__func__, dwc->maximum_speed, dwc->has_hibernation,
			dwc->nr_scratch);

	ret = dwc3_alloc_scratch_buffers(dwc);
	if (ret)
		goto err1;

	ret = dwc3_setup_scratch_buffers(dwc);
	if (ret)
		goto err2;

	dwc3_core_config(dwc);

	if (dwc->adj_sof_accuracy) {
		reg = dwc3_readl(dwc->regs, DWC3_GFLADJ);
		reg &= ~DWC3_GFLADJ_REFCLK_240MHZDECR_PLS1;
		reg &= ~DWC3_GFLADJ_REFCLK_240MHZ_DECR_MASK;
		reg |= DWC3_GFLADJ_REFCLK_240MHZ_DECR(0xA);
		reg |= DWC3_GFLADJ_REFCLK_LPM_SEL;
		reg &= ~DWC3_GFLADJ_REFCLK_FLADJ_MASK;
		reg |= DWC3_GFLADJ_REFCLK_FLADJ(0x7F0);
		dwc3_writel(dwc->regs, DWC3_GFLADJ, reg);
	}

	return 0;

err2:
	dwc3_free_scratch_buffers(dwc);

err1:
	usb_phy_shutdown(dwc->usb2_phy);
	usb_phy_shutdown(dwc->usb3_phy);
	phy_exit(dwc->usb2_generic_phy);
	phy_exit(dwc->usb3_generic_phy);

	phy_power_off(dwc->usb2_generic_phy);
	phy_power_off(dwc->usb3_generic_phy);
err0:
	return ret;
}

void dwc3_core_exit(struct dwc3 *dwc)
{
	dwc3_free_scratch_buffers(dwc);
	usb_phy_shutdown(dwc->usb2_phy);
	usb_phy_shutdown(dwc->usb3_phy);
	phy_exit(dwc->usb2_generic_phy);
	phy_exit(dwc->usb3_generic_phy);

	phy_power_off(dwc->usb2_generic_phy);
	phy_power_off(dwc->usb3_generic_phy);
}

static int dwc3_core_get_phy(struct dwc3 *dwc)
{
	struct device		*dev = dwc->dev;
	struct device_node	*node = dev->of_node;
	int ret;

	if (node) {
		dwc->usb2_phy = devm_usb_get_phy_by_phandle(dev, "usb-phy", 0);
		dwc->usb3_phy = devm_usb_get_phy_by_phandle(dev, "usb-phy", 1);
	} else {
		dwc->usb2_phy = devm_usb_get_phy(dev, USB_PHY_TYPE_USB2);
		dwc->usb3_phy = devm_usb_get_phy(dev, USB_PHY_TYPE_USB3);
	}

	if (IS_ERR(dwc->usb2_phy)) {
		ret = PTR_ERR(dwc->usb2_phy);
		if (ret == -ENXIO || ret == -ENODEV) {
			dwc->usb2_phy = NULL;
		} else if (ret == -EPROBE_DEFER) {
			return ret;
		} else {
			dev_err(dev, "no usb2 phy configured\n");
			return ret;
		}
	}

	if (IS_ERR(dwc->usb3_phy)) {
		ret = PTR_ERR(dwc->usb3_phy);
		if (ret == -ENXIO || ret == -ENODEV) {
			dwc->usb3_phy = NULL;
		} else if (ret == -EPROBE_DEFER) {
			return ret;
		} else {
			dev_err(dev, "no usb3 phy configured\n");
			return ret;
		}
	}

	dwc->usb2_generic_phy = devm_phy_get(dev, "usb2-phy");
	if (IS_ERR(dwc->usb2_generic_phy)) {
		ret = PTR_ERR(dwc->usb2_generic_phy);
		if (ret == -ENOSYS || ret == -ENODEV) {
			dwc->usb2_generic_phy = NULL;
		} else if (ret == -EPROBE_DEFER) {
			return ret;
		} else {
			dev_err(dev, "no usb2 phy configured\n");
			return ret;
		}
	}

	dwc->usb3_generic_phy = devm_phy_get(dev, "usb3-phy");
	if (IS_ERR(dwc->usb3_generic_phy)) {
		ret = PTR_ERR(dwc->usb3_generic_phy);
		if (ret == -ENOSYS || ret == -ENODEV) {
			dwc->usb3_generic_phy = NULL;
		} else if (ret == -EPROBE_DEFER) {
			return ret;
		} else {
			dev_err(dev, "no usb3 phy configured\n");
			return ret;
		}
	}

	return 0;
}

static int dwc3_core_init_mode(struct dwc3 *dwc)
{
	struct device *dev = dwc->dev;
	int ret;

	switch (dwc->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_DEVICE);
		ret = dwc3_gadget_init(dwc);
		if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
			return ret;
		}
		break;
	case USB_DR_MODE_HOST:
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_HOST);
		ret = dwc3_host_init(dwc);
		if (ret) {
			dev_err(dev, "failed to initialize host\n");
			return ret;
		}
		break;
	case USB_DR_MODE_OTG:
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_OTG);
		ret = dwc3_otg_init(dwc);
		if (ret) {
			dev_err(dev, "failed to initialize otg\n");
			return ret;
		}

		/* turn off PHYs to save power */
		dwc3_core_exit(dwc);

		ret = dwc3_host_init(dwc);
		if (ret) {
			dev_err(dev, "failed to initialize host\n");
			dwc3_otg_exit(dwc);
			return ret;
		}
		ret = dwc3_gadget_init(dwc);
		if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
			dwc3_host_exit(dwc);
			dwc3_otg_exit(dwc);
			return ret;
		}

		/* Now we are ready to start OTG */
		ret = dwc3_otg_start(dwc);
		if (ret) {
			dev_err(dev, "failed to start otg\n");
			dwc3_host_exit(dwc);
			dwc3_gadget_exit(dwc);
			dwc3_otg_exit(dwc);
			return ret;
		}

		/* Unblock runtime PM for OTG */
		pm_runtime_put_sync(dev);

		break;
	default:
		dev_err(dev, "Unsupported mode of operation %d\n", dwc->dr_mode);
		return -EINVAL;
	}

	return 0;
}

static void dwc3_core_exit_mode(struct dwc3 *dwc)
{
	switch (dwc->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		dwc3_gadget_exit(dwc);
		break;
	case USB_DR_MODE_HOST:
		dwc3_host_exit(dwc);
		break;
	case USB_DR_MODE_OTG:
		dwc3_otg_stop(dwc);
		dwc3_host_exit(dwc);
		dwc3_gadget_exit(dwc);
		dwc3_otg_exit(dwc);
		break;
	default:
		/* do nothing */
		break;
	}
}

#define DWC3_ALIGN_MASK		(16 - 1)

static int dwc3_probe(struct platform_device *pdev)
{
	struct device		*dev = &pdev->dev;
	struct dwc3_platform_data *pdata = dev_get_platdata(dev);
	struct resource		*res;
	struct dwc3		*dwc;
	u8			lpm_nyet_threshold;
	u8			tx_de_emphasis;
	u8			hird_threshold;
	u32			fladj = 0;

	int			ret;

	void __iomem		*regs;
	void			*mem;

	pr_info("%s: +++\n", __func__);
	mem = devm_kzalloc(dev, sizeof(*dwc) + DWC3_ALIGN_MASK, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	dwc = PTR_ALIGN(mem, DWC3_ALIGN_MASK + 1);
	dwc->mem = mem;
	dwc->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(dev, "missing IRQ\n");
		return -ENODEV;
	}
	dwc->xhci_resources[1].start = res->start;
	dwc->xhci_resources[1].end = res->end;
	dwc->xhci_resources[1].flags = res->flags;
	dwc->xhci_resources[1].name = res->name;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "missing memory resource\n");
		return -ENODEV;
	}

	dwc->xhci_resources[0].start = res->start;
	dwc->xhci_resources[0].end = dwc->xhci_resources[0].start +
					DWC3_XHCI_REGS_END;
	dwc->xhci_resources[0].flags = res->flags;
	dwc->xhci_resources[0].name = res->name;

	res->start += DWC3_GLOBALS_REGS_START;

	/*
	 * Request memory region but exclude xHCI regs,
	 * since it will be requested by the xhci-plat driver.
	 */
	regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto err0;
	}

	dwc->regs	= regs;
	dwc->regs_size	= resource_size(res);

	/* default to highest possible threshold */
	lpm_nyet_threshold = 0xff;

	/* default to -3.5dB de-emphasis */
	tx_de_emphasis = 1;

	/*
	 * default to assert utmi_sleep_n and use maximum allowed HIRD
	 * threshold value of 0b1100
	 */
	hird_threshold = 12;

	dwc->maximum_speed = usb_get_maximum_speed(dev);
	dwc->dr_mode = usb_get_dr_mode(dev);
	dwc->suspend_clk_freq = of_usb_get_suspend_clk_freq(dev);
	dwc->adj_sof_accuracy = device_property_read_bool(dev,
				"adj-sof-accuracy");

	dwc->has_lpm_erratum = device_property_read_bool(dev,
				"snps,has-lpm-erratum");
	device_property_read_u8(dev, "snps,lpm-nyet-threshold",
				&lpm_nyet_threshold);
	dwc->is_utmi_l1_suspend = device_property_read_bool(dev,
				"snps,is-utmi-l1-suspend");
	device_property_read_u8(dev, "snps,hird-threshold",
				&hird_threshold);
	dwc->usb3_lpm_capable = device_property_read_bool(dev,
				"snps,usb3_lpm_capable");

	dwc->needs_fifo_resize = device_property_read_bool(dev,
				"tx-fifo-resize");
	dwc->is_not_vbus_pad = device_property_read_bool(dev,
				"is_not_vbus_pad");
	dwc->sparse_transfer_control = device_property_read_bool(dev,
				"enable_sprs_transfer");
	dwc->disable_scramble_quirk = device_property_read_bool(dev,
				"snps,disable_scramble_quirk");
	dwc->u2exit_lfps_quirk = device_property_read_bool(dev,
				"snps,u2exit_lfps_quirk");
	dwc->u2ss_inp3_quirk = device_property_read_bool(dev,
				"snps,u2ss_inp3_quirk");
	dwc->req_p1p2p3_quirk = device_property_read_bool(dev,
				"snps,req_p1p2p3_quirk");
	dwc->del_p1p2p3_quirk = device_property_read_bool(dev,
				"snps,del_p1p2p3_quirk");
	dwc->del_phy_power_chg_quirk = device_property_read_bool(dev,
				"snps,del_phy_power_chg_quirk");
	dwc->lfps_filter_quirk = device_property_read_bool(dev,
				"snps,lfps_filter_quirk");
	dwc->rx_detect_poll_quirk = device_property_read_bool(dev,
				"snps,rx_detect_poll_quirk");
	dwc->dis_u3_susphy_quirk = device_property_read_bool(dev,
				"snps,dis_u3_susphy_quirk");
	dwc->dis_u2_susphy_quirk = device_property_read_bool(dev,
				"snps,dis_u2_susphy_quirk");
	dwc->dis_enblslpm_quirk = device_property_read_bool(dev,
				"snps,dis_enblslpm_quirk");

	dwc->tx_de_emphasis_quirk = device_property_read_bool(dev,
				"snps,tx_de_emphasis_quirk");
	device_property_read_u8(dev, "snps,tx_de_emphasis",
				&tx_de_emphasis);
	device_property_read_string(dev, "snps,hsphy_interface",
				    &dwc->hsphy_interface);
	device_property_read_u32(dev, "snps,quirk-frame-length-adjustment",
				 &fladj);

	if (pdata) {
		dwc->maximum_speed = pdata->maximum_speed;
		dwc->has_lpm_erratum = pdata->has_lpm_erratum;
		if (pdata->lpm_nyet_threshold)
			lpm_nyet_threshold = pdata->lpm_nyet_threshold;
		dwc->is_utmi_l1_suspend = pdata->is_utmi_l1_suspend;
		if (pdata->hird_threshold)
			hird_threshold = pdata->hird_threshold;

		dwc->needs_fifo_resize = pdata->tx_fifo_resize;
		dwc->usb3_lpm_capable = pdata->usb3_lpm_capable;
		dwc->sparse_transfer_control = pdata->sparse_transfer_control;
		dwc->is_not_vbus_pad = pdata->is_not_vbus_pad;
		dwc->dr_mode = pdata->dr_mode;
		dwc->suspend_clk_freq = pdata->suspend_clk_freq;
		dwc->adj_sof_accuracy = pdata->adj_sof_accuracy;

		dwc->disable_scramble_quirk = pdata->disable_scramble_quirk;
		dwc->u2exit_lfps_quirk = pdata->u2exit_lfps_quirk;
		dwc->u2ss_inp3_quirk = pdata->u2ss_inp3_quirk;
		dwc->req_p1p2p3_quirk = pdata->req_p1p2p3_quirk;
		dwc->del_p1p2p3_quirk = pdata->del_p1p2p3_quirk;
		dwc->del_phy_power_chg_quirk = pdata->del_phy_power_chg_quirk;
		dwc->lfps_filter_quirk = pdata->lfps_filter_quirk;
		dwc->rx_detect_poll_quirk = pdata->rx_detect_poll_quirk;
		dwc->dis_u3_susphy_quirk = pdata->dis_u3_susphy_quirk;
		dwc->dis_u2_susphy_quirk = pdata->dis_u2_susphy_quirk;
		dwc->dis_enblslpm_quirk = pdata->dis_enblslpm_quirk;

		dwc->tx_de_emphasis_quirk = pdata->tx_de_emphasis_quirk;
		if (pdata->tx_de_emphasis)
			tx_de_emphasis = pdata->tx_de_emphasis;

		dwc->hsphy_interface = pdata->hsphy_interface;
		fladj = pdata->fladj_value;
	}

	dev_info(dwc->dev, "%s: dr_mode:%d, suspend clock:%dMHz\n", __func__,
			dwc->dr_mode , dwc->suspend_clk_freq/1000000);

	/* default to superspeed if no maximum_speed passed */
	if (dwc->maximum_speed == USB_SPEED_UNKNOWN)
		dwc->maximum_speed = USB_SPEED_SUPER;

	dwc->lpm_nyet_threshold = lpm_nyet_threshold;
	dwc->tx_de_emphasis = tx_de_emphasis;

	dwc->hird_threshold = hird_threshold
		| (dwc->is_utmi_l1_suspend << 4);

	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);
	pm_runtime_forbid(dev);

	platform_set_drvdata(pdev, dwc);
	dwc3_cache_hwparams(dwc);

	ret = dwc3_phy_setup(dwc);
	if (ret)
		goto err0;

	ret = dwc3_core_get_phy(dwc);
	if (ret)
		goto err0;

	spin_lock_init(&dwc->lock);
	init_completion(&dwc->disconnect);

#if IS_ENABLED(CONFIG_ZONE_DMA)
	if (!dev->dma_mask) {
#endif
		dev->dma_mask = dev->parent->dma_mask;
		dev->dma_parms = dev->parent->dma_parms;
		dma_set_coherent_mask(dev, dev->parent->coherent_dma_mask);
#if IS_ENABLED(CONFIG_ZONE_DMA)
	}
#endif

	ret = dwc3_alloc_event_buffers(dwc, DWC3_EVENT_BUFFERS_SIZE);
	if (ret) {
		dev_err(dwc->dev, "failed to allocate event buffers\n");
		ret = -ENOMEM;
		goto err1;
	}

	if (IS_ENABLED(CONFIG_USB_DWC3_HOST))
		dwc->dr_mode = USB_DR_MODE_HOST;
	else if (IS_ENABLED(CONFIG_USB_DWC3_GADGET))
		dwc->dr_mode = USB_DR_MODE_PERIPHERAL;

	if (dwc->dr_mode == USB_DR_MODE_UNKNOWN)
		dwc->dr_mode = USB_DR_MODE_OTG;

	ret = dwc3_core_init(dwc);
	if (ret) {
		dev_err(dev, "failed to initialize core\n");
		goto err1;
	}

	/* Adjust Frame Length */
	dwc3_frame_length_adjustment(dwc, fladj);

	usb_phy_set_suspend(dwc->usb2_phy, 0);
	usb_phy_set_suspend(dwc->usb3_phy, 0);

	ret = dwc3_event_buffers_setup(dwc);
	if (ret) {
		dev_err(dwc->dev, "failed to setup event buffers\n");
		goto err4;
	}

	ret = dwc3_core_init_mode(dwc);
	if (ret)
		goto err5;

	ret = dwc3_debugfs_init(dwc);
	if (ret) {
		dev_err(dev, "failed to initialize debugfs\n");
		goto err6;
	}

	pm_runtime_allow(dev);
	INIT_WORK(&dwc->set_vbus_current_work, dwc3_exynos_set_vbus_current_work);

	pr_info("%s: ---\n", __func__);
	return 0;

err6:
	dwc3_core_exit_mode(dwc);

err5:
	dwc3_event_buffers_cleanup(dwc);

err4:
	usb_phy_set_suspend(dwc->usb2_phy, 1);
	usb_phy_set_suspend(dwc->usb3_phy, 1);
	dwc3_core_exit(dwc);

err1:
	dwc3_free_event_buffers(dwc);
	dwc3_ulpi_exit(dwc);

err0:
	/*
	 * restore res->start back to its original value so that, in case the
	 * probe is deferred, we don't end up getting error in request the
	 * memory region the next time probe is called.
	 */
	res->start -= DWC3_GLOBALS_REGS_START;

	return ret;
}

static int dwc3_remove(struct platform_device *pdev)
{
	struct dwc3	*dwc = platform_get_drvdata(pdev);
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	/*
	 * restore res->start back to its original value so that, in case the
	 * probe is deferred, we don't end up getting error in request the
	 * memory region the next time probe is called.
	 */
	res->start -= DWC3_GLOBALS_REGS_START;

	dwc3_debugfs_exit(dwc);
	dwc3_core_exit_mode(dwc);
	dwc3_event_buffers_cleanup(dwc);
	dwc3_free_event_buffers(dwc);

	usb_phy_set_suspend(dwc->usb2_phy, 1);
	usb_phy_set_suspend(dwc->usb3_phy, 1);
	phy_power_off(dwc->usb2_generic_phy);
	phy_power_off(dwc->usb3_generic_phy);

	dwc3_core_exit(dwc);
	dwc3_ulpi_exit(dwc);

	if (dwc->dr_mode != USB_DR_MODE_OTG)
		pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int dwc3_suspend(struct device *dev)
{
	struct dwc3	*dwc = dev_get_drvdata(dev);
	unsigned long	flags;

	/* bring to full power */
	pm_runtime_get_sync(dev);

	if (dwc->dr_mode == USB_DR_MODE_OTG)
		dwc3_otg_stop(dwc);

	spin_lock_irqsave(&dwc->lock, flags);

	switch (dwc->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		dwc3_gadget_suspend(dwc);
		break;
	case USB_DR_MODE_OTG:
		dwc3_event_buffers_cleanup(dwc);
		break;
	case USB_DR_MODE_HOST:
		dwc3_event_buffers_cleanup(dwc);
		usb_phy_shutdown(dwc->usb3_phy);
		usb_phy_shutdown(dwc->usb2_phy);
		phy_exit(dwc->usb2_generic_phy);
		phy_exit(dwc->usb3_generic_phy);

		phy_power_off(dwc->usb2_generic_phy);
		phy_power_off(dwc->usb3_generic_phy);
	default:
		/* do nothing */
		break;
	}

	/* backup GCTL only in non-OTG modes */
	if (dwc->dr_mode != USB_DR_MODE_OTG)
		dwc->gctl = dwc3_readl(dwc->regs, DWC3_GCTL);

	spin_unlock_irqrestore(&dwc->lock, flags);

	pinctrl_pm_select_sleep_state(dev);

	return 0;
}

static int dwc3_resume(struct device *dev)
{
	struct dwc3	*dwc = dev_get_drvdata(dev);
	unsigned long	flags;
	int		ret;

	pinctrl_pm_select_default_state(dev);

	spin_lock_irqsave(&dwc->lock, flags);

	dwc3_event_buffers_setup(dwc);

	if (dwc->dr_mode != USB_DR_MODE_OTG)
		dwc3_writel(dwc->regs, DWC3_GCTL, dwc->gctl);

	switch (dwc->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		dwc3_gadget_resume(dwc);
		break;
	case USB_DR_MODE_OTG:
		break;
	case USB_DR_MODE_HOST:
		ret = phy_power_on(dwc->usb3_generic_phy);
		if (ret < 0) {
			spin_unlock_irqrestore(&dwc->lock, flags);
			return ret;
		}
		ret = phy_power_on(dwc->usb2_generic_phy);
		if (ret < 0)
		    goto err_usb3phy_power;

		usb_phy_init(dwc->usb3_phy);
		usb_phy_init(dwc->usb2_phy);
		ret = phy_init(dwc->usb2_generic_phy);
		if (ret < 0)
			goto err_usb2phy_power;

		ret = phy_init(dwc->usb3_generic_phy);
		if (ret < 0)
			goto err_usb2phy_init;
	default:
		/* do nothing */
		break;
	}

	spin_unlock_irqrestore(&dwc->lock, flags);

	pm_runtime_disable(dev);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	if (dwc->dr_mode == USB_DR_MODE_OTG) {
#if defined(CONFIG_USB_HOST_SAMSUNG_FEATURE)
		dwc3_otg_ready(dwc);
#else
		dwc3_otg_start(dwc);
#endif
	}

	/* Compensate usage count incremented during prepare */
	pm_runtime_put_sync(dev);

	return 0;

err_usb2phy_init:
	phy_exit(dwc->usb2_generic_phy);
err_usb2phy_power:
	phy_power_off(dwc->usb2_generic_phy);

err_usb3phy_power:
	phy_power_off(dwc->usb3_generic_phy);

	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static const struct dev_pm_ops dwc3_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dwc3_suspend, dwc3_resume)
};

#define DWC3_PM_OPS	&(dwc3_dev_pm_ops)
#else
#define DWC3_PM_OPS	NULL
#endif

#ifdef CONFIG_OF
static const struct of_device_id of_dwc3_match[] = {
	{
		.compatible = "snps,dwc3"
	},
	{
		.compatible = "synopsys,dwc3"
	},
	{ },
};
MODULE_DEVICE_TABLE(of, of_dwc3_match);
#endif

#ifdef CONFIG_ACPI

#define ACPI_ID_INTEL_BSW	"808622B7"

static const struct acpi_device_id dwc3_acpi_match[] = {
	{ ACPI_ID_INTEL_BSW, 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, dwc3_acpi_match);
#endif

static struct platform_driver dwc3_driver = {
	.probe		= dwc3_probe,
	.remove		= dwc3_remove,
	.driver		= {
		.name	= "dwc3",
		.of_match_table	= of_match_ptr(of_dwc3_match),
		.acpi_match_table = ACPI_PTR(dwc3_acpi_match),
		.pm	= DWC3_PM_OPS,
	},
};

module_platform_driver(dwc3_driver);

MODULE_ALIAS("platform:dwc3");
MODULE_AUTHOR("Felipe Balbi <balbi@ti.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DesignWare USB3 DRD Controller Driver");
