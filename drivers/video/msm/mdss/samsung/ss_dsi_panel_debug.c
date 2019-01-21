/*
 * =================================================================
 *
 *	Description:  samsung display debug common file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2015, Samsung Electronics. All rights reserved.

*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
*/
#include "ss_dsi_panel_common.h"
#include "../mdss_debug.h"

#define BF_TYPE 0x4D42             /* "MB" */

struct BITMAPFILEHEADER                 /**** BMP file header structure ****/
{
	unsigned short bfType;           /* Magic number for file */
	unsigned int   bfSize;           /* Size of file */
	unsigned short bfReserved1;      /* Reserved */
	unsigned short bfReserved2;      /* ... */
	unsigned int   bfOffBits;        /* Offset to bitmap data */
	unsigned int   biSize;           /* Size of info header */
	int            biWidth;          /* Width of image */
	int            biHeight;         /* Height of image */
	unsigned short biPlanes;         /* Number of color planes */
	unsigned short biBitCount;       /* Number of bits per pixel */
	unsigned int   biCompression;    /* Type of compression to use */
	unsigned int   biSizeImage;      /* Size of image data */
	int            biXPelsPerMeter;  /* X pixels per meter */
	int            biYPelsPerMeter;  /* Y pixels per meter */
	unsigned int   biClrUsed;        /* Number of colors used */
	unsigned int   biClrImportant;   /* Number of important colors */
} __packed;

/************************************************************
*
*		MDSS & DSI REGISTER DUMP FUNCTION
*
**************************************************************/
size_t kvaddr_to_paddr(unsigned long vaddr)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	size_t paddr;

	pgd = pgd_offset_k(vaddr);
	if (unlikely(pgd_none(*pgd) || pgd_bad(*pgd)))
		return 0;

	pud = pud_offset(pgd, vaddr);
	if (unlikely(pud_none(*pud) || pud_bad(*pud)))
		return 0;

	pmd = pmd_offset(pud, vaddr);
	if (unlikely(pmd_none(*pmd) || pmd_bad(*pmd)))
		return 0;

	pte = pte_offset_kernel(pmd, vaddr);
	if (!pte_present(*pte))
		return 0;

	paddr = (unsigned long)pte_pfn(*pte) << PAGE_SHIFT;
	paddr += (vaddr & (PAGE_SIZE - 1));

	return paddr;
}

static void dump_reg(char *addr, int len)
{
	bool v_in_interrupt;

	if (IS_ERR_OR_NULL(addr))
		return;

	v_in_interrupt = in_interrupt() ? 1 : 0;
	mdss_dump_reg("ss_dump", MDSS_DBG_DUMP_IN_LOG, addr, len, NULL, v_in_interrupt);
}

void mdss_samsung_dump_regs(void)
{
	struct mdss_data_type *mdata = mdss_mdp_get_mdata();
	struct mdss_mdp_ctl *ctl;
	char name[32];
	int loop;
	int max_rects;

	if (IS_ERR_OR_NULL(mdata))
		return;
	else
		 ctl = mdata->ctl_off;

	snprintf(name, sizeof(name), "MDP BASE");
	LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)mdata->mdss_io.base));
	dump_reg(mdata->mdss_io.base, 0x100);

	snprintf(name, sizeof(name), "MDP REG");
	LCD_ERR("=============%s 0x%08zx ==============\n", name,
		kvaddr_to_paddr((unsigned long)mdata->mdp_base));
	dump_reg(mdata->mdp_base, 0x500);

	for (loop = 0; loop < mdata->nctl ; loop++) {
		snprintf(name, sizeof(name), "CTRL%d", loop);
		LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)mdata->ctl_off[loop].base));
		dump_reg(mdata->ctl_off[loop].base, 0x100);
	}

	if (mdata->nvig_pipes) {
		max_rects = mdata->vig_pipes[0].multirect.max_rects;

		for (loop = 0; loop < mdata->nvig_pipes ; loop++) {
			snprintf(name, sizeof(name), "VG%d", loop);
			LCD_ERR("=============%s 0x%08zx ==============\n", name,
				kvaddr_to_paddr((unsigned long)mdata->vig_pipes[loop * max_rects].base));
			/* dump_reg(mdata->vig_pipes[loop].base, 0x100); */
			dump_reg(mdata->vig_pipes[loop].base, 0x400);
		}
	}

	if (mdata->nrgb_pipes) {
		max_rects = mdata->rgb_pipes[0].multirect.max_rects;

		for (loop = 0; loop < mdata->nrgb_pipes ; loop++) {
			snprintf(name, sizeof(name), "RGB%d", loop);
			LCD_ERR("=============%s 0x%08zx ==============\n", name,
				kvaddr_to_paddr((unsigned long)mdata->rgb_pipes[loop * max_rects].base));
			/* dump_reg(mdata->rgb_pipes[loop].base, 0x100); */
			dump_reg(mdata->rgb_pipes[loop].base, 0x400);
		}
	}

	if (mdata->ndma_pipes) {
		max_rects = mdata->dma_pipes[0].multirect.max_rects;

		for (loop = 0; loop < mdata->ndma_pipes ; loop++) {
			snprintf(name, sizeof(name), "DMA%d", loop);
			LCD_ERR("=============%s 0x%08zx ==============\n", name,
				kvaddr_to_paddr((unsigned long)mdata->dma_pipes[loop * max_rects].base));
			/* dump_reg(mdata->dma_pipes[loop].base, 0x100); */
			dump_reg(mdata->dma_pipes[loop].base, 0x400);
		}
	}

	for (loop = 0; loop < mdata->nmixers_intf ; loop++) {
		snprintf(name, sizeof(name), "MIXER_INTF_%d", loop);
		LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)mdata->mixer_intf[loop].base));
		dump_reg(mdata->mixer_intf[loop].base, 0x100);
	}

	for (loop = 0; loop < mdata->nmixers_wb ; loop++) {
		snprintf(name, sizeof(name), "MIXER_WB_%d", loop);
		LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)mdata->mixer_wb[loop].base));
		dump_reg(mdata->mixer_wb[loop].base, 0x100);
	}

	if (ctl->is_video_mode) {
		for (loop = 0; loop < mdata->nintf; loop++) {
			snprintf(name, sizeof(name), "VIDEO_INTF%d", loop);
			LCD_ERR("=============%s 0x%08zx ==============\n", name,
				kvaddr_to_paddr((unsigned long)mdss_mdp_get_intf_base_addr(mdata, loop)));
			dump_reg(mdss_mdp_get_intf_base_addr(mdata, loop), 0x40);
		}
	}

	for (loop = 0; loop < mdata->nmixers_intf ; loop++) {
		snprintf(name, sizeof(name), "PING_PONG%d", loop);
		LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)mdata->mixer_intf[loop].pingpong_base));
		/* dump_reg(mdata->mixer_intf[loop].pingpong_base, 0x40); */
		dump_reg(mdata->mixer_intf[loop].pingpong_base, 0x100);
	}

	for (loop = 0; loop < mdata->nwb; loop++) {
		snprintf(name, sizeof(name), "WRITE BACK%d", loop);
		LCD_ERR("=============%s 0x%08zx ==============\n", name,
				kvaddr_to_paddr((unsigned long)mdata->mdss_io.base + mdata->wb_offsets[loop]));
		dump_reg((mdata->mdss_io.base + mdata->wb_offsets[loop]), 0x2BC);
	}

	if (ctl->mfd->panel_info->compression_mode == COMPRESSION_DSC) {
		for (loop = 0; loop < mdata->ndsc ; loop++) {
			snprintf(name, sizeof(name), "DSC%d", loop);
			LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)mdata->dsc_off[loop].base));
			dump_reg(mdata->dsc_off[loop].base, 0x200);
		}
	}

#if defined(CONFIG_ARCH_MSM8992)
	/* To dump ping-pong slave register for ping-pong split supporting chipset */
	snprintf(name, sizeof(name), "PING_PONG SLAVE");
		LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)mdata->slave_pingpong_base));
		dump_reg(mdata->slave_pingpong_base, 0x40);
#endif

	snprintf(name, sizeof(name), "VBIF");
	LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)mdata->vbif_io.base));
	dump_reg(mdata->vbif_io.base, 0x270);

	snprintf(name, sizeof(name), "VBIF NRT");
	LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)mdata->vbif_nrt_io.base));
	dump_reg(mdata->vbif_nrt_io.base, 0x270);

}

void mdss_samsung_dsi_dump_regs(struct samsung_display_driver_data *vdd, int dsi_num)
{
	struct mdss_dsi_ctrl_pdata *dsi_ctrl = NULL;
	char name[32];

	dsi_ctrl = vdd->ctrl_dsi[dsi_num];
	if (!dsi_ctrl) {
		LCD_ERR("dsi_ctrl is null.. (%d)\n", dsi_num);
		return;
	}

	if (vdd->panel_attach_status & BIT(dsi_num)) {
		snprintf(name, sizeof(name), "DSI%d CTL", dsi_num);
		LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)dsi_ctrl->ctrl_io.base));
		dump_reg((char *)dsi_ctrl->ctrl_io.base, dsi_ctrl->ctrl_io.len);

		snprintf(name, sizeof(name), "DSI%d PHY", dsi_num);
		LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)dsi_ctrl->phy_io.base));
		dump_reg((char *)dsi_ctrl->phy_io.base, (size_t)dsi_ctrl->phy_io.len);

		snprintf(name, sizeof(name), "DSI%d PLL", dsi_num);
		LCD_ERR("=============%s 0x%08zx ==============\n", name,
			kvaddr_to_paddr((unsigned long)vdd->dump_info[dsi_num].dsi_pll.virtual_addr));
		dump_reg((char *)vdd->dump_info[dsi_num].dsi_pll.virtual_addr, 0x200);

#if defined(CONFIG_ARCH_MSM8992) || defined(CONFIG_ARCH_MSM8994)
		if (dsi_ctrl->shared_ctrl_data->phy_regulator_io.base) {
			snprintf(name, sizeof(name), "DSI%d REGULATOR", dsi_num);
			LCD_ERR("=============%s 0x%08zx ==============\n", name,
				kvaddr_to_paddr((unsigned long)dsi_ctrl->shared_ctrl_data->phy_regulator_io.base));
			dump_reg((char *)dsi_ctrl->shared_ctrl_data->phy_regulator_io.base, (size_t)dsi_ctrl->shared_ctrl_data->phy_regulator_io.len);
		}
#endif
	}
}

int mdss_samsung_read_rddpm(struct samsung_display_driver_data *vdd)
{
	struct mdss_dsi_ctrl_pdata *dsi_ctrl = NULL;
	char rddpm = 0;

	dsi_ctrl = samsung_get_dsi_ctrl(vdd);
	if (!dsi_ctrl) {
		LCD_ERR("dsi_ctrl is null.. \n");
		return 0;
	}

	MDSS_XLOG(dsi_ctrl->ndx, 0x0A);

	if (!IS_ERR_OR_NULL(get_panel_rx_cmds(dsi_ctrl, RX_LDI_DEBUG0)->cmds)) {
		mdss_samsung_read_nv_mem(dsi_ctrl, get_panel_rx_cmds(dsi_ctrl, RX_LDI_DEBUG0), &rddpm, LEVEL_KEY_NONE);

		LCD_DEBUG("========== SHOW PANEL [0Ah:RDDPM] INFO ==========\n");
		LCD_DEBUG("* Reg Value : 0x%02x, Result : %s\n",
					rddpm, (rddpm == 0x9C) ? "GOOD" : "NG");
		LCD_DEBUG("* Bootster Mode : %s\n", rddpm & 0x80 ? "ON (GD)" : "OFF (NG)");
		LCD_DEBUG("* Idle Mode     : %s\n", rddpm & 0x40 ? "ON (NG)" : "OFF (GD)");
		LCD_DEBUG("* Partial Mode  : %s\n", rddpm & 0x20 ? "ON" : "OFF");
		LCD_DEBUG("* Sleep Mode    : %s\n", rddpm & 0x10 ? "OUT (GD)" : "IN (NG)");
		LCD_DEBUG("* Normal Mode   : %s\n", rddpm & 0x08 ? "OK (GD)" : "SLEEP (NG)");
		LCD_DEBUG("* Display ON    : %s\n", rddpm & 0x04 ? "ON (GD)" : "OFF (NG)");
		LCD_DEBUG("=================================================\n");

		if (rddpm == 0x08) {
			LCD_ERR("ddi reset status (%x)\n", rddpm);
			/* It used to use to recover the panel at display unblank time.
			 * schedule_work(&pstatus_data->check_status.work);
			 */
			return 1;
		}
	} else
		LCD_ERR("no rddpm read cmds..\n");

	return 0;
}

int mdss_samsung_read_rddsm(struct samsung_display_driver_data *vdd)
{
	struct mdss_dsi_ctrl_pdata *dsi_ctrl = NULL;
	char rddsm = 0;

	dsi_ctrl = samsung_get_dsi_ctrl(vdd);
	if (!dsi_ctrl) {
		LCD_ERR("dsi_ctrl is null.. \n");
		return 0;
	}

	MDSS_XLOG(dsi_ctrl->ndx, 0x0E);

	if (!IS_ERR_OR_NULL(get_panel_rx_cmds(dsi_ctrl, RX_LDI_DEBUG3)->cmds)) {
		mdss_samsung_read_nv_mem(dsi_ctrl, get_panel_rx_cmds(dsi_ctrl, RX_LDI_DEBUG3), &rddsm, LEVEL_KEY_NONE);

		LCD_DEBUG("========== SHOW PANEL [0Eh:RDDSM] INFO ==========\n");
		LCD_DEBUG("* Reg Value : 0x%02x, Result : %s\n",
					rddsm, (rddsm == 0x80) ? "GOOD" : "NG");
		LCD_DEBUG("* TE Mode : %s\n", rddsm & 0x80 ? "ON(GD)" : "OFF(NG)");
		LCD_DEBUG("=================================================\n");

	} else
		LCD_ERR("no rddsm read cmds..\n");

	return 0;
}

int mdss_samsung_read_errfg(struct samsung_display_driver_data *vdd)
{
	struct mdss_dsi_ctrl_pdata *dsi_ctrl = NULL;
	char err_fg = 0;

	dsi_ctrl = samsung_get_dsi_ctrl(vdd);
	if (!dsi_ctrl) {
		LCD_ERR("dsi_ctrl is null.. \n");
		return 0;
	}

	MDSS_XLOG(dsi_ctrl->ndx, 0xEE);

	if (!IS_ERR_OR_NULL(get_panel_rx_cmds(dsi_ctrl, RX_LDI_DEBUG2)->cmds)) {
		mdss_samsung_read_nv_mem(dsi_ctrl, get_panel_rx_cmds(dsi_ctrl, RX_LDI_DEBUG2), &err_fg, LEVEL2_KEY);

		LCD_DEBUG("========== SHOW PANEL [EEh:ERR_FG] INFO ==========\n");
		LCD_DEBUG("* Reg Value : 0x%02x, Result : %s\n",
					err_fg, (err_fg & 0x4C) ? "NG" : "GOOD");

		if (err_fg & 0x04) {
			LCD_ERR("* VLOUT3 Error\n");
			inc_dpui_u32_field(DPUI_KEY_PNVLO3E, 1);
		}

		if (err_fg & 0x08) {
			LCD_ERR("* ELVDD Error\n");
			inc_dpui_u32_field(DPUI_KEY_PNELVDE, 1);
		}

		if (err_fg & 0x40) {
			LCD_ERR("* VLIN1 Error\n");
			inc_dpui_u32_field(DPUI_KEY_PNVLI1E, 1);
		}
		LCD_DEBUG("==================================================\n");

		inc_dpui_u32_field(DPUI_KEY_PNESDE, (err_fg & 0x4D) ? 1 : 0);
	} else
		LCD_ERR("no errfg read cmds..\n");

	return 0;
}

int mdss_samsung_read_dsierr(struct samsung_display_driver_data *vdd)
{
	struct mdss_dsi_ctrl_pdata *dsi_ctrl = NULL;
	char dsi_err = 0;

	dsi_ctrl = samsung_get_dsi_ctrl(vdd);
	if (!dsi_ctrl) {
		LCD_ERR("dsi_ctrl is null.. \n");
		return 0;
	}

	MDSS_XLOG(dsi_ctrl->ndx, 0x05);

	if (!IS_ERR_OR_NULL(get_panel_rx_cmds(dsi_ctrl, RX_LDI_DEBUG4)->cmds)) {
		mdss_samsung_read_nv_mem(dsi_ctrl, get_panel_rx_cmds(dsi_ctrl, RX_LDI_DEBUG4), &dsi_err, LEVEL1_KEY);

		LCD_DEBUG("========== SHOW PANEL [05h:DSIE_CNT] INFO ==========\n");
		LCD_DEBUG("* Reg Value : 0x%02x, Result : %s\n",
				dsi_err, (dsi_err) ? "NG" : "GOOD");
		if (dsi_err)
			LCD_ERR("* DSI Error Count : %d\n", dsi_err);
		LCD_DEBUG("====================================================\n");

		inc_dpui_u32_field(DPUI_KEY_PNDSIE, dsi_err);
	} else
		LCD_ERR("no dsi err read cmds..\n");

	return 0;
}

int mdss_samsung_read_self_diag(struct samsung_display_driver_data *vdd)
{
	struct mdss_dsi_ctrl_pdata *dsi_ctrl = NULL;
	char self_diag = 0;

	dsi_ctrl = samsung_get_dsi_ctrl(vdd);
	if (!dsi_ctrl) {
		LCD_ERR("dsi_ctrl is null.. \n");
		return 0;
	}

	MDSS_XLOG(dsi_ctrl->ndx, 0x0F);

	if (!IS_ERR_OR_NULL(get_panel_rx_cmds(dsi_ctrl, RX_LDI_DEBUG5)->cmds)) {
		mdss_samsung_read_nv_mem(dsi_ctrl, get_panel_rx_cmds(dsi_ctrl, RX_LDI_DEBUG5), &self_diag, LEVEL_KEY_NONE);

		LCD_DEBUG("========== SHOW PANEL [0Fh:SELF_DIAG] INFO ==========\n");
		LCD_DEBUG("* Reg Value : 0x%02x, Result : %s\n",
				self_diag, (self_diag & 0x80) ? "GOOD" : "NG");
		if ((self_diag & 0x80) == 0)
			LCD_ERR("* OTP Reg Loading Error\n");
		LCD_DEBUG("=====================================================\n");

		inc_dpui_u32_field(DPUI_KEY_PNSDRE, (self_diag & 0x80) ? 0 : 1);
	} else
		LCD_ERR("no OPT loading read cmds..\n");

	return 0;
}

int mdss_samsung_dsi_te_check(struct samsung_display_driver_data *vdd)
{
	struct mdss_dsi_ctrl_pdata *dsi_ctrl = NULL;
	int rc, te_count = 0;
	int te_max = 20000; /*sampling 200ms */

	dsi_ctrl = vdd->ctrl_dsi[DSI_CTRL_0];
	if (!dsi_ctrl) {
		LCD_ERR("dsi_ctrl is null.. (%d)\n", DSI_CTRL_0);
		return 0;
	}

	if (dsi_ctrl->panel_mode == DSI_VIDEO_MODE)
		return 0;

	if (gpio_is_valid(dsi_ctrl->disp_te_gpio)) {
		LCD_ERR("============ start waiting for TE ============\n");

		for (te_count = 0;  te_count < te_max; te_count++) {
			rc = gpio_get_value(dsi_ctrl->disp_te_gpio);
			if (rc == 1) {
				LCD_ERR("gpio_get_value(disp_te_gpio) = %d ",
									rc);
				LCD_ERR("te_count = %d\n", te_count);
				break;
			}
			/* usleep suspends the calling thread whereas udelay is a
			 * busy wait. Here the value of te_gpio is checked in a loop of
			 * max count = 250. If this loop has to iterate multiple
			 * times before the te_gpio is 1, the calling thread will end
			 * up in suspend/wakeup sequence multiple times if usleep is
			 * used, which is an overhead. So use udelay instead of usleep.
			 */
			udelay(10);
		}

		if (te_count == te_max) {
			LCD_ERR("LDI doesn't generate TE, ddi recovery start.");
			return 1;
		} else
			LCD_ERR("LDI generate TE\n");

		LCD_ERR("============ finish waiting for TE ============\n");
	} else
		LCD_ERR("disp_te_gpio is not valid\n");

	return 0;
}

void mdss_mdp_underrun_dump_info(struct samsung_display_driver_data *vdd)
{
	struct mdss_mdp_pipe *pipe;
	/* struct mdss_data_type *mdss_res = mdss_mdp_get_mdata(); */
	struct mdss_overlay_private *mdp5_data = mfd_to_mdp5_data(vdd->mfd_dsi[0]);
	int pcount = mdp5_data->mdata->nrgb_pipes + mdp5_data->mdata->nvig_pipes + mdp5_data->mdata->ndma_pipes;

	LCD_ERR(" ============ start ===========\n");
	list_for_each_entry(pipe, &mdp5_data->pipes_used, list) {
		if (pipe && pipe->src_fmt)
			LCD_ERR("[%4d, %4d, %4d, %4d] -> [%4d, %4d, %4d, %4d]"
				"|flags = %8x|src_format = %2d|bpp = %2d|ndx = %3d|\n",
				pipe->src.x, pipe->src.y, pipe->src.w, pipe->src.h,
				pipe->dst.x, pipe->dst.y, pipe->dst.w, pipe->dst.h,
				pipe->flags, pipe->src_fmt->format, pipe->src_fmt->bpp,
				pipe->ndx);
		LCD_ERR("pipe addr : %p\n", pipe);
		pcount--;
		if (!pcount)
			break;
	}
/*
	LCD_ERR("mdp_clk = %ld, bus_ab = %llu, bus_ib = %llu\n", mdss_mdp_get_clk_rate(MDSS_CLK_MDP_SRC),
		mdss_res->bus_scale_table->usecase[mdss_res->curr_bw_uc_idx].vectors[0].ab,
		mdss_res->bus_scale_table->usecase[mdss_res->curr_bw_uc_idx].vectors[0].ib);
*/
	LCD_ERR("============ end ===========\n");
}

#if 0
DEFINE_MUTEX(FENCE_LOCK);
static const char *sync_status_str(int status)
{
	if (status > 0)
		return "signaled";
	else if (status == 0)
		return "active";
	else
		return "error";
}

static void sync_pt_log(struct sync_pt *pt, bool pt_callback)
{
	int status = pt->status;

	pr_cont("[mdss DEBUG_FENCE]  %s_pt %s",
		   pt->parent->name,
		   sync_status_str(status));

	if (pt->status) {
		struct timeval tv = ktime_to_timeval(pt->timestamp);

		pr_cont("@%ld.%06ld", tv.tv_sec, tv.tv_usec);
	}

	if (pt->parent->ops->timeline_value_str &&
	    pt->parent->ops->pt_value_str) {
		char value[64];

		pt->parent->ops->pt_value_str(pt, value, sizeof(value));
		pr_cont(": %s", value);
		pt->parent->ops->timeline_value_str(pt->parent, value,
					    sizeof(value));
		pr_cont(" / %s", value);
	}

	pr_cont("\n");

	/* Show additional details for active fences */
	if (pt->status == 0 && pt->parent->ops->pt_log && pt_callback)
		pt->parent->ops->pt_log(pt);
}

void mdss_samsung_fence_dump(char *intf, struct sync_fence *fence)
{
	struct sync_pt *pt;
	struct list_head *pos;

	mutex_lock(&FENCE_LOCK);

	LCD_ERR("[mdss DEBUG_FENCE] %s : %s start\n", intf, fence->name);

	list_for_each(pos, &fence->pt_list_head) {
		pt = container_of(pos, struct sync_pt, pt_list);
		sync_pt_log(pt, true);
	}

	LCD_ERR("[mdss DEBUG_FENCE] %s : %s end\n", intf, fence->name);

	mutex_unlock(&FENCE_LOCK);
}
#endif


/************************************************************
*
*		IMAGE DUMP FUNCTION
*
**************************************************************/

#include "../../../../staging/android/ion/ion_priv.h"
#include "../../../../staging/android/ion/ion.h"

struct ion_handle {
	struct kref ref;
	struct ion_client *client;
	struct ion_buffer *buffer;
	struct rb_node node;
	unsigned int kmap_cnt;
	int id;
};

static int WIDTH_COMPRESS_RATIO = 1;
static int HEIGHT_COMPRESS_RATIO = 1;

static inline char samsung_drain_image_888(struct mdss_mdp_pipe *pipe, char *drain_pos, int drain_x, int drain_y, int RGB)
{
	char (*drain_pos_RGB888)[pipe->img_width][3] = (void *)drain_pos;

	return drain_pos_RGB888[drain_y][drain_x][RGB];
}

static inline char samsung_drain_image_8888(struct mdss_mdp_pipe *pipe, char *drain_pos, int drain_x, int drain_y, int RGB)
{
	char (*drain_pos_RGB8888)[pipe->img_width][4] = (void *)drain_pos;

	return drain_pos_RGB8888[drain_y][drain_x][RGB];
}

void samsung_image_dump(struct samsung_display_driver_data *vdd)
{
	struct mdss_panel_info *panel_info = &vdd->ctrl_dsi[DISPLAY_1]->panel_data.panel_info;
	struct mdss_overlay_private *mdp5_data = mfd_to_mdp5_data(vdd->mfd_dsi[0]);
	struct mdss_mdp_pipe *pipe;
	struct mdss_mdp_data *buf;

	int z_order;

	int x, y;
	int dst_x, dst_y;
	int drain_x, drain_y, drain_max_x, drain_max_y;
	int panel_x_res, panel_y_res;

	char *dst_fb_pos = NULL;
	char *drain_pos = NULL;

	char (*dst_fb_pos_none_split)[panel_info->xres / WIDTH_COMPRESS_RATIO][3];
	char (*dst_fb_pos_split)[(panel_info->xres * 2) / WIDTH_COMPRESS_RATIO][3];

	struct ion_buffer *ion_buffers;

	int split_mode, acquire_lock;
	/*
		cont_splash_mem: cont_splash_mem@0 {
			linux,reserve-contiguous-region;
			linux,reserve-region;
			reg = <0 0x83000000 0 0x01C00000>;
			label = "cont_splash_mem";
		};

		QC reserves tripple buffer for WQHD size at kernel. 0x2200000 is 34Mbyte.
		So we uses second buffer to image dump.
	*/
	unsigned int buffer_offset, buffer_size;
	struct BITMAPFILEHEADER bitmap;
	int bitmap_y_reverse;

	LCD_ERR("start\n");

	if (vdd->mfd_dsi[0]->split_mode == MDP_DUAL_LM_DUAL_DISPLAY ||
		vdd->mfd_dsi[0]->split_mode == MDP_PINGPONG_SPLIT)  {
		split_mode = true;
		buffer_offset = ((panel_info->xres * 2) * panel_info->yres * 3);
		panel_x_res = (panel_info->xres * 2) / WIDTH_COMPRESS_RATIO;
		panel_y_res = panel_info->yres / HEIGHT_COMPRESS_RATIO;
		buffer_size = panel_x_res * panel_y_res * 3;
	} else {
		split_mode = false;
		buffer_offset = (panel_info->xres * panel_info->yres * 3);
		panel_x_res = panel_info->xres / WIDTH_COMPRESS_RATIO;
		panel_y_res = panel_info->yres / HEIGHT_COMPRESS_RATIO;
		buffer_size = panel_x_res * panel_y_res * 3;
	}

	acquire_lock = mutex_trylock(&mdp5_data->list_lock);

	/*
		We use 2 frame to dump image.

		First, dump image to First fb frame address

		Second, copy to reversed data to Second fb Frame address from Frist fb frame address.
		Because BMP format use upside-down & flipped format
	*/

	/* BMP HEADER ADDRESS */
	dst_fb_pos = phys_to_virt(mdp5_data->splash_mem_addr + buffer_offset);

	if (IS_ERR_OR_NULL(dst_fb_pos))
		return;

	/* Generate BMP format */
	bitmap.bfType = BF_TYPE;
	bitmap.bfSize = buffer_size + sizeof(struct BITMAPFILEHEADER);
	bitmap.bfReserved1 = 0x00;
	bitmap.bfReserved2 = 0x00;
	bitmap.bfOffBits = sizeof(struct BITMAPFILEHEADER);
	bitmap.biSize = 0x28;
	bitmap.biWidth = panel_x_res;
	bitmap.biHeight = panel_y_res;
	bitmap.biPlanes = 0x01;
	bitmap.biBitCount = 0x18;
	bitmap.biCompression = 0x00;
	bitmap.biSizeImage = buffer_size;
	bitmap.biXPelsPerMeter = (1000 * panel_info->xres) / panel_info->physical_width;
	bitmap.biYPelsPerMeter = (1000 * panel_info->yres) / panel_info->physical_height;
	bitmap.biClrUsed = 0x00;
	bitmap.biClrImportant = 0x00;

	memcpy(dst_fb_pos, &bitmap, sizeof(struct BITMAPFILEHEADER));

	/* fb frameaddress */
	dst_fb_pos = phys_to_virt(mdp5_data->splash_mem_addr + buffer_offset + sizeof(struct BITMAPFILEHEADER));

	if (IS_ERR_OR_NULL(dst_fb_pos))
		return;

	dst_fb_pos_none_split = (void *)dst_fb_pos;
	dst_fb_pos_split = (void *)dst_fb_pos;
	memset(dst_fb_pos, 0x00, buffer_size);

	LCD_ERR("dst_pos : 0x%p split_mode : %s xres :%d yres :%d\n", dst_fb_pos,
		vdd->mfd_dsi[0]->split_mode == 0 ? "MDP_SPLIT_MODE_NONE" :
		vdd->mfd_dsi[0]->split_mode == 1 ? "MDP_DUAL_LM_SINGLE_DISPLAY" :
		vdd->mfd_dsi[0]->split_mode == 2 ? "MDP_DUAL_LM_DUAL_DISPLAY" : "MDP_SPLIT_MODE_NONE",
		panel_info->xres, panel_info->yres);

	for (z_order = MDSS_MDP_STAGE_0; z_order < MDSS_MDP_MAX_STAGE; z_order++) {
		list_for_each_entry(pipe, &mdp5_data->pipes_used, list) {
			/* Check z-order */
			if (pipe->mixer_stage != z_order)
				continue;

			LCD_ERR("L_mixer:%d R_mixer:%d %s z:%d format:%d %s flag:0x%x src.x:%d y:%d w:%d h:%d "
				"des_rect.x:%d y:%d w:%d h:%d\n",
				pipe->mixer_left ? pipe->mixer_left->num : -1,
				pipe->mixer_right ? pipe->mixer_right->num : -1,
				pipe->ndx == BIT(0) ? "VG0" : pipe->ndx == BIT(1) ? "VG1" :
				pipe->ndx == BIT(2) ? "VG2" : pipe->ndx == BIT(3) ? "RGB0" :
				pipe->ndx == BIT(4) ? "RGB1" : pipe->ndx == BIT(5) ? "RGB2" :
				pipe->ndx == BIT(6) ? "DMA0" : pipe->ndx == BIT(7) ? "DMA1" :
				pipe->ndx == BIT(8) ? "VG3" : pipe->ndx == BIT(9) ? "RGB3" :
				pipe->ndx == BIT(10) ? "CURSOR0" : pipe->ndx == BIT(11) ? "CURSOR1" : "MAX_SSPP",
				pipe->mixer_stage - MDSS_MDP_STAGE_0,
				pipe->src_fmt->format,
				pipe->src_fmt->format == MDP_RGB_888 ? "MDP_RGB_888" :
				pipe->src_fmt->format == MDP_RGBX_8888 ? "MDP_RGBX_8888" :
				pipe->src_fmt->format == MDP_Y_CRCB_H2V2 ? "MDP_Y_CRCB_H2V2" :
				pipe->src_fmt->format == MDP_RGBA_8888 ? "MDP_RGBA_8888" :
				pipe->src_fmt->format == MDP_Y_CBCR_H2V2_VENUS ? "MDP_Y_CBCR_H2V2_VENUS" : "NONE",
				pipe->flags,
				pipe->src.x, pipe->src.y, pipe->src.w, pipe->src.h,
				pipe->dst.x, pipe->dst.y, pipe->dst.w, pipe->dst.h);

			/* Check format */
			if (pipe->src_fmt->format != MDP_RGB_888 &&
					pipe->src_fmt->format != MDP_RGBA_8888 &&
					pipe->src_fmt->format != MDP_RGBX_8888)
				continue;

			buf = list_first_entry_or_null(&pipe->buf_queue,
					struct mdss_mdp_data, pipe_list);

			if (IS_ERR_OR_NULL(buf))
				continue;

#if 1 /*defined(CONFIG_ARCH_MSM8996) || defined(CONFIG_ARCH_MSM8937) || defined(CONFIG_ARCH_MSM8953)*/
			ion_buffers = (struct ion_buffer *)(buf->p[0].srcp_dma_buf->priv);
#else
			ion_buffers = buf->p[0].srcp_ihdl->buffer;
#endif
			if (!IS_ERR_OR_NULL(ion_buffers)) {
				/* To avoid buffer free */
				kref_get(&ion_buffers->ref);

				drain_pos = ion_heap_map_kernel(NULL, ion_buffers);

				if (!IS_ERR_OR_NULL(drain_pos)) {
					dst_x = pipe->dst.x / WIDTH_COMPRESS_RATIO;
					dst_y = pipe->dst.y / HEIGHT_COMPRESS_RATIO;

					drain_x = pipe->src.x;
					drain_y = pipe->src.y;

					drain_max_x = dst_x + (pipe->dst.w / WIDTH_COMPRESS_RATIO);
					drain_max_y = dst_y + (pipe->dst.h / HEIGHT_COMPRESS_RATIO);

					/*
						Because of BMP format, we save RGB to BGR & Flip(up -> donw) image
					*/
					if (pipe->src_fmt->format == MDP_RGB_888) {
						if (split_mode) {
							for (y = dst_y; y < drain_max_y; y++, drain_y += HEIGHT_COMPRESS_RATIO) {
								bitmap_y_reverse = panel_y_res -  y;
								drain_x = pipe->src.x;
								for (x = dst_x; x < drain_max_x; x++, drain_x += WIDTH_COMPRESS_RATIO) {
									dst_fb_pos_split[bitmap_y_reverse][x][0] |= samsung_drain_image_888(pipe, drain_pos, drain_x, drain_y, 2); /* B */
									dst_fb_pos_split[bitmap_y_reverse][x][1] |= samsung_drain_image_888(pipe, drain_pos, drain_x, drain_y, 1); /* G */
									dst_fb_pos_split[bitmap_y_reverse][x][2] |= samsung_drain_image_888(pipe, drain_pos, drain_x, drain_y, 0); /* R */
								}
							}

						} else {
							for (y = dst_y; y < drain_max_y; y++, drain_y += HEIGHT_COMPRESS_RATIO) {
								bitmap_y_reverse = panel_y_res -  y;
								drain_x = pipe->src.x;
								for (x = dst_x; x < drain_max_x; x++, drain_x += WIDTH_COMPRESS_RATIO) {
									dst_fb_pos_none_split[bitmap_y_reverse][x][0] |= samsung_drain_image_888(pipe, drain_pos, drain_x, drain_y, 2); /* B */
									dst_fb_pos_none_split[bitmap_y_reverse][x][1] |= samsung_drain_image_888(pipe, drain_pos, drain_x, drain_y, 1); /* G */
									dst_fb_pos_none_split[bitmap_y_reverse][x][2] |= samsung_drain_image_888(pipe, drain_pos, drain_x, drain_y, 0); /* R */
								}
							}
						}
					} else {
						if (split_mode) {
							for (y = dst_y; y < drain_max_y; y++, drain_y += HEIGHT_COMPRESS_RATIO) {
								bitmap_y_reverse = panel_y_res -  y;
								drain_x = pipe->src.x;
								for (x = dst_x; x < drain_max_x; x++, drain_x += WIDTH_COMPRESS_RATIO) {
									dst_fb_pos_split[bitmap_y_reverse][x][0] |= samsung_drain_image_8888(pipe, drain_pos, drain_x, drain_y, 2); /* B */
									dst_fb_pos_split[bitmap_y_reverse][x][1] |= samsung_drain_image_8888(pipe, drain_pos, drain_x, drain_y, 1); /* G */
									dst_fb_pos_split[bitmap_y_reverse][x][2] |= samsung_drain_image_8888(pipe, drain_pos, drain_x, drain_y, 0); /* R */
								}
							}
						} else {
							for (y = dst_y; y < drain_max_y; y++, drain_y += HEIGHT_COMPRESS_RATIO) {
								bitmap_y_reverse = panel_y_res -  y;
								drain_x = pipe->src.x;
								for (x = dst_x; x < drain_max_x; x++, drain_x += WIDTH_COMPRESS_RATIO) {
									dst_fb_pos_none_split[bitmap_y_reverse][x][0] |= samsung_drain_image_8888(pipe, drain_pos, drain_x, drain_y, 2); /* B */
									dst_fb_pos_none_split[bitmap_y_reverse][x][1] |= samsung_drain_image_8888(pipe, drain_pos, drain_x, drain_y, 1); /* G */
									dst_fb_pos_none_split[bitmap_y_reverse][x][2] |= samsung_drain_image_8888(pipe, drain_pos, drain_x, drain_y, 0); /* R */
								}
							}
						}
					}

					ion_heap_unmap_kernel(NULL, ion_buffers);
				} else
					LCD_ERR("vmap fail");
			} else
				LCD_ERR("ion_buffer is NULL\n");
		}
	}

	if (acquire_lock)
		mutex_unlock(&mdp5_data->list_lock);

	LCD_ERR("end\n");
}

void samsung_image_dump_worker(struct samsung_display_driver_data *vdd, struct work_struct *work)
{
	struct mdss_panel_info *panel_info = &vdd->ctrl_dsi[DISPLAY_1]->panel_data.panel_info;
	struct mdss_overlay_private *mdp5_data = mfd_to_mdp5_data(vdd->mfd_dsi[0]);
	unsigned int image_size;

	/* update compress ratio */
	if (vdd->mfd_dsi[0]->split_mode == MDP_DUAL_LM_DUAL_DISPLAY ||
		vdd->mfd_dsi[0]->split_mode == MDP_PINGPONG_SPLIT)
		image_size = (panel_info->xres * 2) * panel_info->yres * 3;
	else
		image_size = panel_info->xres * panel_info->yres * 3;

	if (image_size <= SZ_2M) {
		WIDTH_COMPRESS_RATIO = 1;
		HEIGHT_COMPRESS_RATIO = 1;
	} else if (image_size <= SZ_8M) {
		WIDTH_COMPRESS_RATIO = 2;
		HEIGHT_COMPRESS_RATIO = 2;
	} else {
		WIDTH_COMPRESS_RATIO = 4;
		HEIGHT_COMPRESS_RATIO = 4;
	}

	/* Clear image dump BMP format */
	memset(phys_to_virt(mdp5_data->splash_mem_addr + image_size), 0x00, sizeof(struct BITMAPFILEHEADER));

	if (in_interrupt()) {
		LCD_ERR("in_interrupt()\n");
		return;
	}

	/* real dump */
	samsung_image_dump(vdd);
}

void samsung_mdss_image_dump(void)
{
	static int dump_done;
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

#if defined(CONFIG_SEC_DEBUG)
	if (!sec_debug_is_enabled())
		return;
#endif

	if (!vdd) {
		LCD_ERR("VDD is null..\n");
		return;
	}

	if (dump_done)
		return;
	else
		dump_done = true;

	if (in_interrupt()) {
		if (!IS_ERR_OR_NULL(vdd->image_dump_workqueue)) {
			queue_work(vdd->image_dump_workqueue, &vdd->image_dump_work);
			LCD_ERR("dump_work queued\n");
		}
	} else
		samsung_image_dump_worker(vdd, NULL);
}


static ssize_t mdss_samsung_dump_regs_debug(struct file *file,
			char __user *buff, size_t count, loff_t *ppos)
{
	struct samsung_display_driver_data *vdd = file->private_data;

	if (IS_ERR_OR_NULL(vdd))
		return -EFAULT;

	mdss_samsung_read_rddpm(vdd);
	mdss_samsung_dump_regs();
	mdss_samsung_dsi_dump_regs(vdd, DSI_CTRL_0);
	mdss_samsung_dsi_dump_regs(vdd, DSI_CTRL_1);
	mdss_samsung_dsi_te_check(vdd);
	mdss_mdp_underrun_dump_info(vdd);

	return 0;
}

static struct file_operations panel_dump_ops = {
	.open = simple_open,
	.read = mdss_samsung_dump_regs_debug,
};

/*
 * Debugfs related functions
 */
static ssize_t mdss_samsung_panel_lpm_ctrl_debug(struct file *file,
		    const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct samsung_display_driver_data *vdd = file->private_data;
	struct mdss_panel_data *pdata = NULL;
	struct mdss_dsi_ctrl_pdata *ctrl;
	int mode, ret = count;
	char buf[10];
	size_t buf_size = min(count, sizeof(buf) - 1);

	if (IS_ERR_OR_NULL(vdd)) {
		ret = -EFAULT;
		goto end;
	}

	if (copy_from_user(buf, user_buf, buf_size)) {
		ret = -EFAULT;
		goto end;
	}

	if (sscanf(buf, "%d", &mode) != 1) {
		ret = -EFAULT;
		goto end;
	}

	ctrl = vdd->ctrl_dsi[DSI_CTRL_0];
	pdata = &ctrl->panel_data;

	if (!vdd->dtsi_data[DISPLAY_1].panel_lpm_enable) {
		LCD_INFO("[Panel LPM DEBUG] panel_lpm is not supported \n");
		goto end;
	}

	LCD_INFO("[Panel LPM DEBUG] Mode : %d\n", mode);
	mdss_samsung_panel_lpm_mode_store(pdata, (u8)(mode + MAX_LPM_MODE));

	if ((vdd->panel_lpm.hz == TX_LPM_2HZ) ||
			(vdd->panel_lpm.hz == TX_LPM_1HZ))
			mdss_samsung_panel_lpm_hz_ctrl(pdata, TX_LPM_AOD_OFF);

	if (vdd->panel_lpm.mode != MODE_OFF) {
		mdss_samsung_send_cmd(ctrl, TX_LPM_ON);
		LCD_INFO("[Panel LPM] Send panel LPM cmds\n");

		mdss_samsung_panel_lpm_hz_ctrl(pdata, TX_LPM_HZ_NONE);
		vdd->display_status_dsi[ctrl->ndx].wait_disp_on = true;
		LCD_DEBUG("[Panel LPM] Set wait_disp_on to true\n");
	} else if (vdd->panel_lpm.mode == MODE_OFF) {
		/* Turn Off ALPM Mode */
		mdss_samsung_send_cmd(ctrl, TX_LPM_OFF);
		LCD_INFO("[Panel LPM] Send panel LPM off cmds\n");

		LCD_DEBUG("[Panel LPM] Restore brightness level\n");
		mutex_lock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
		pdata->set_backlight(pdata, vdd->mfd_dsi[DISPLAY_1]->bl_level);
		mutex_unlock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
		vdd->display_status_dsi[ctrl->ndx].wait_disp_on = true;
	}

	if ((vdd->panel_lpm.hz == TX_LPM_2HZ) ||
			(vdd->panel_lpm.hz == TX_LPM_1HZ))
			mdss_samsung_panel_lpm_hz_ctrl(pdata, TX_LPM_AOD_ON);

end:
	return ret;
}

static struct file_operations panel_lpm_ops = {
	.open = simple_open,
	.write = mdss_samsung_panel_lpm_ctrl_debug,
};

static ssize_t mdss_samsung_panel_revision_write(struct file *file,
		    const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct samsung_display_driver_data *vdd = file->private_data;
	int ret = count;
	char revision;
	int buf_size;
	char buf[10];

	if (IS_ERR_OR_NULL(vdd)) {
		ret = -EFAULT;
		goto end;
	}

	buf_size = min(count, (sizeof(buf) - 1));
	if (copy_from_user(buf, user_buf, buf_size)) {
		ret = -EFAULT;
		goto end;
	}

	buf[buf_size] = '\0';
	if (sscanf(buf, "%c", &revision) != 1) {
		ret = -EFAULT;
		goto end;
	}

	switch (revision) {
	case 'A' ... 'Z':
		vdd->panel_revision = revision - 'A';
		break;
	case 'a' ... 'z':
		vdd->panel_revision = revision - 'a';
		break;
	default:
		LCD_INFO("Invalid panel revision(%c)\n", revision);
		break;
	}

	LCD_INFO("Panel revision(%c)\n", vdd->panel_revision + 'A');

end:
	return ret;
}

static ssize_t mdss_samsung_panel_revision_read(struct file *file,
			char __user *buff, size_t count, loff_t *ppos)
{
	struct samsung_display_driver_data *vdd = file->private_data;
	int ret;
	char buf[30] = "Panel Revision : ";
	int len = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		ret = -EFAULT;
		goto end;
	}

	len = strlen(buf);
	buf[len++] = vdd->panel_revision + 'A';
	buf[len++] = '\n';
	buf[len++] = 0x00;

	return simple_read_from_buffer(buff, count, ppos, buf, len);
end:
	return 0;
}


static struct file_operations panel_revision_ops = {
	.open = simple_open,
	.write = mdss_samsung_panel_revision_write,
	.read = mdss_samsung_panel_revision_read,
};

static void mdss_sasmung_panel_debug_create(struct samsung_display_driver_data *vdd)
{
	struct samsung_display_debug_data *debug_data;

	debug_data = vdd->debug_data;

	/* Create file on debugfs of display_driver */


	/* Create file on debugfs on dump */
	debugfs_create_file("reg_dump", 0600, debug_data->dump,	vdd, &panel_dump_ops);
	debugfs_create_bool("print_cmds", 0600, debug_data->dump,
		(u32 *)&debug_data->print_cmds);
	debugfs_create_bool("panic_on_pptimeout", 0600, debug_data->dump,
		(u32 *)&debug_data->panic_on_pptimeout);

	/* Create file on debugfs on display_status */
	debugfs_create_u32("panel_attach_status", 0600, debug_data->display_status,
		(u32 *)&vdd->panel_attach_status);

	debugfs_create_file("panel_lpm", 0600, debug_data->display_status,
		vdd, &panel_lpm_ops);

	debugfs_create_file("panel_revision", 0600, debug_data->display_status,
		vdd, &panel_revision_ops);

	if (!IS_ERR_OR_NULL(debug_data->is_factory_mode))
		debugfs_create_bool("is_factory_mode", 0600, debug_data->root,
			(u32 *)debug_data->is_factory_mode);

	/* Create file on debugfs on hw_info */
	/* TBD */
}

int mdss_sasmung_panel_debug_init(struct samsung_display_driver_data *vdd)
{
	struct samsung_display_debug_data *debug_data;
	static bool debugfs_init;
	int ret = 0;

	debug_data = kzalloc(sizeof(struct samsung_display_debug_data),
			GFP_KERNEL);

	if (IS_ERR_OR_NULL(vdd)) {
		ret = -ENODEV;
		goto end;
	}

	/*
	 * The debugfs must be init one time
	 * in case of dual dsi, this function will be called twice
	 */
	if (debugfs_init)
		goto end;

	debugfs_init = true;

	vdd->debug_data = debug_data;

	if (IS_ERR_OR_NULL(debug_data)) {
		LCD_ERR("no memory to create display debug data\n");
		ret = -ENOMEM;
		goto end;
	}

	/* INIT debug data */
	debug_data->is_factory_mode = &vdd->is_factory_mode;

	/*
	 * panic_on_pptimeout default value is false
	 * if you want to enable panic for specific project
	 * please change the value on your panel file.
	 * if you want to enable panic for all project
	 * please change the value here.
	 */
	debug_data->panic_on_pptimeout = false;

	/* Root directory for display driver */
	debug_data->root = debugfs_create_dir("display_driver", NULL);
	if (IS_ERR_OR_NULL(debug_data->root)) {
		LCD_ERR("debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(debug_data->root), __LINE__);
		ret = -ENODEV;
		goto end;
	}

	/* Directory for dump */
	debug_data->dump = debugfs_create_dir("dump", debug_data->root);
	if (IS_ERR_OR_NULL(debug_data->dump)) {
		LCD_ERR("debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(debug_data->dump), __LINE__);
		ret = -ENODEV;
		goto end;
	}

	/* Directory for hw_info */
	debug_data->hw_info = debugfs_create_dir("hw_info", debug_data->root);
	if (IS_ERR_OR_NULL(debug_data->root)) {
		LCD_ERR("debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(debug_data->root), __LINE__);
		ret = -ENODEV;
		goto end;
	}

	/* Directory for hw_info */
	debug_data->display_status = debugfs_create_dir("display_status", debug_data->root);
	if (IS_ERR_OR_NULL(debug_data->display_status)) {
		LCD_ERR("debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(debug_data->root), __LINE__);
		ret = -ENODEV;
		goto end;
	}

	mdss_sasmung_panel_debug_create(vdd);

	if (ret)
		LCD_ERR("Fail to create files for debugfs\n");

end:
	if (ret && !IS_ERR_OR_NULL(debug_data->root))
			debugfs_remove_recursive(debug_data->root);

	return ret;
}
