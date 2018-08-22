#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/namei.h>
#include <linux/security.h>

#include "regs/iva_base_addr.h"
#include "iva_ram_dump.h"
#include "iva_mbox.h"
#include "iva_mcu.h"
#include "iva_mem.h"
#include "iva_pmu.h"

#define CMSG_REQ_FATAL_HEADER		(0x1)
#define MAX_DUMP_PATH_SIZE		(256)

#define DFT_RAMDUMP_OUT_DIR		"/data/media/0/DCIM/"
#define DFT_RAMDUMP_OUTPUT(f)		(DFT_RAMDUMP_OUT_DIR "/" f)

#define RAMDUMP_DATE_FORMAT		"YYYYMMDD.HHMMSS"
#define RAMDUMP_FILENAME_PREFIX		"iva_ramdump"
#define RAMDUMP_FILENAME_EXT		"bin"

#define VCM_DBG_SEL_SAVE_OFFSET		(IVA_MBOX_BASE_ADDR)
#define VCM_DBG_SEL_SAVE_MAX_SIZE	(IVA_HWA_ADDR_GAP)

struct iva_mmr_skip_range_info {
	uint32_t	from;
	uint32_t	to;
};

typedef int32_t (*prepare_fn)(struct iva_dev_data *iva);

struct iva_mmr_sect_info {
	const char	*name;
	uint32_t	base;
	uint32_t	size;
	const uint32_t	skip_num;
	const struct iva_mmr_skip_range_info *skip_info;
	prepare_fn	pre_func;
};

static const  struct iva_mmr_sect_info iva_sfr_pmu = {
	.name = "pmu",
	.base = IVA_PMU_BASE_ADDR,
	.size = IVA_HWA_ADDR_GAP,
};

static const  struct iva_mmr_sect_info iva_sfr_mcu = {
	.name = "mcu",
	.base = IVA_VMCU_MEM_BASE_ADDR,
	.size = VMCU_MEM_SIZE,
};

static const struct iva_mmr_skip_range_info vcm_skip_range[] = {
	{	/* handled in vcm_sbuf section */
		.from	= IVA_VCM_SCH_BUF_OFFSET,
		.to	= IVA_VCM_SCH_BUF_OFFSET + IVA_VCM_SCH_BUF_SIZE - 1
	}, {	/* handled in vcm_cbuf section */
		.from	= IVA_VCM_CMD_BUF_OFFSET,
		.to	= IVA_VCM_CMD_BUF_OFFSET + IVA_VCM_CMD_BUF_SIZE - 1
	},
#if defined(CONFIG_SOC_EXYNOS9810)
	{	/* last 8 bytes */
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};

static int32_t iva_ram_dump_prepare_vcm_buf(struct iva_dev_data *iva)
{
	iva_pmu_reset_hwa(iva, pmu_vcm_id, false);
	iva_pmu_reset_hwa(iva, pmu_vcm_id, true);
	return 0;
}

#if defined(CONFIG_SOC_EXYNOS9810)
static const struct iva_mmr_skip_range_info orb_skip_range[] = {
	{ .from = 0xfff8, .to = 0xfffc }, /* last 4 bytes */
};

static const struct iva_mmr_skip_range_info lmd_skip_range[] = {
	{ .from = 0xfffc, .to = 0xfffc }, /* last 4 bytes */
};
#endif

static const struct iva_mmr_sect_info iva_sfr_sects[] = {
	/*
	 * causion: do not add mbox if there is no specific reason.
	 */
	{
		.name = "vcf",
		.base = IVA_VCF_BASE_ADDR,
		.size = VCF_MEM_SIZE,
	}, {
		.name = "vcm",
		.base = IVA_VCM_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = vcm_skip_range,
		.skip_num = (uint32_t) ARRAY_SIZE(vcm_skip_range),
	}, {	/* sub range in vcm section : sync with vcm_skip_range */
		.name = "vcm_sbuf",
		.base = IVA_VCM_BASE_ADDR + IVA_VCM_SCH_BUF_OFFSET,
		.size = IVA_VCM_SCH_BUF_SIZE,
		.pre_func = iva_ram_dump_prepare_vcm_buf,
	}, {	/*
		 * sub range in vcm section : sync with vcm_skip_range
		 * always behind vcm_sbuf.
		 */
		.name = "vcm_cbuf",
		.base = IVA_VCM_BASE_ADDR + IVA_VCM_CMD_BUF_OFFSET,
		.size = IVA_VCM_CMD_BUF_SIZE,
	}, {
		.name = "csc",
		.base = IVA_CSC_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "scl0",
		.base = IVA_SCL0_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "scl1",
		.base = IVA_SCL1_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "orb",
		.base = IVA_ORB_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	#if defined(CONFIG_SOC_EXYNOS9810)
		.skip_info = orb_skip_range,
		.skip_num = (uint32_t) ARRAY_SIZE(orb_skip_range),
	#endif
	}, {
		.name = "mch",
		.base = IVA_MCH_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "lmd",
		.base = IVA_LMD_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	#if defined(CONFIG_SOC_EXYNOS9810)
		.skip_info = lmd_skip_range,
		.skip_num = (uint32_t) ARRAY_SIZE(lmd_skip_range),
	#endif
	}, {
		.name = "lkt",
		.base = IVA_LKT_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "wig0",
		.base = IVA_WIG0_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "wig1",
		.base = IVA_WIG1_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "enf",
		.base = IVA_ENF_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
#if defined(CONFIG_SOC_EXYNOS8895)
		.name = "vdma0",
#elif defined(CONFIG_SOC_EXYNOS9810)
		.name = "vdma",
#endif
		.base = IVA_VDMA0_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	},
#if defined(CONFIG_SOC_EXYNOS8895)
	{
		.name = "vdma1",
		.base = IVA_VDMA1_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}
#elif defined(CONFIG_SOC_EXYNOS9810)
	{
		.name = "bax",
		.base = IVA_BAX_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "mot",
		.base = IVA_MOT_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "bld",
		.base = IVA_BLD_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "dif",
		.base = IVA_DIF_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "wig2",
		.base = IVA_WIG2_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}, {
		.name = "wig3",
		.base = IVA_WIG3_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}
#endif
};

static void iva_ram_dump_copy_section_sfrs(struct iva_dev_data *iva,
		u32 *dst_sect_addr, const struct iva_mmr_sect_info *sect_info)
{
	u32 __iomem	*src_sfr_base;
	uint32_t	i, skip_idx;
	u32		size_u32 = (u32) sizeof(u32);
	u32		align_mask = (u32)(~(size_u32 - 1));
	u32		aligned_from, aligned_to;

	if (!sect_info) {
		dev_warn(iva->dev, "%s() null sect_info.\n", __func__);
		return;
	}

	if (!dst_sect_addr) {
		dev_warn(iva->dev, "%s() %s: null dst_sect_addr.\n", __func__,
				sect_info->name);
		return;
	}

	src_sfr_base = (u32 __iomem *) (iva->iva_va + sect_info->base);

	/* init memory with 0xfc */
	memset(dst_sect_addr, 0xfc, sect_info->size);

	dev_dbg(iva->dev, "%s() start to save %s from %p (size: 0x%x)\n",
			__func__, sect_info->name, src_sfr_base, sect_info->size);

	/* call prepare function to secure dump its hwa's sfrs */
	if (sect_info->pre_func)
		sect_info->pre_func(iva);

	for (i = 0; i < sect_info->skip_num; i++) {
		aligned_from	= sect_info->skip_info[i].from & align_mask;
		aligned_to	= sect_info->skip_info[i].to & align_mask;

		/* check boundary */
		if ((aligned_from > aligned_to) ||
				aligned_from >= sect_info->size ||
				aligned_to >= sect_info->size) {
			dev_warn(iva->dev, "%s() %s has incorrect skip boundary",
					__func__, sect_info->name);
			dev_warn(iva->dev, "(0x%x - 0x%x) size(0x%x)\n",
					aligned_from, aligned_to,
					sect_info->size);
			continue;
		}

		/* mark skip area */
		for (skip_idx = aligned_from / size_u32;
				skip_idx <= aligned_to / size_u32; skip_idx++) {
			dst_sect_addr[skip_idx] = 0x0;
		}
	}

	for (i = 0; i < sect_info->size / size_u32; i++) {
		if (dst_sect_addr[i] == 0x0)	/* skip */
			continue;
		dst_sect_addr[i] = __raw_readl(src_sfr_base + i);
	}
}

void iva_ram_dump_copy_vcm_dbg(struct iva_dev_data *iva,
		void *dst_buf, uint32_t dst_buf_sz)
{
	#define VCM_LOG_LINE_LENGTH	(64)
	#define VCM_DGBSEL_REG		(0xFFF0)
	#define ENABLE_CAPTURE_M	(0x1 << 16)
	#define VCM_DGBVAL_REG		(0xFFF4)
	const uint32_t vcm_dbg_mux_sel[] = {
#if defined(CONFIG_SOC_EXYNOS8895)
		0x03, 0x04, 0x07, 0x08, 0x0b, 0x0c, 0x0f, 0x10
#elif defined(CONFIG_SOC_EXYNOS9810)
		0x00, 0x01,
		0x04, 0x05,	/* sched 0 */
		0x08, 0x09,	/* sched 1 */
		0x0C, 0x0D,	/* sched 2 */
		0x10, 0x11,	/* sched 3 */
		0x14, 0x15,	/* sched 4 */
		0x18, 0x19,	/* sched 5 */
		0x1C, 0x1D,	/* sched 7 */
		0x1E,
		0x1F,
#endif
	};

	uint32_t	i;
	int		copy_size = 0;
	u32		reg_val;
	char		*dst_str = (char *) dst_buf;

	dev_dbg(iva->dev, "start to save vcm_dbg.\n");

	for (i = 0; i < (uint32_t) ARRAY_SIZE(vcm_dbg_mux_sel); i++) {
		writel(ENABLE_CAPTURE_M | vcm_dbg_mux_sel[i],
				iva->iva_va + IVA_VCM_BASE_ADDR + VCM_DGBSEL_REG);
		reg_val = readl(iva->iva_va + IVA_VCM_BASE_ADDR + VCM_DGBVAL_REG);
		copy_size += snprintf(dst_str + copy_size,
				dst_buf_sz - copy_size - 1 /* including null*/,
				"DBGSEL[%02x]=0x%08x\n",
				vcm_dbg_mux_sel[i], reg_val);
	}

	if (copy_size != 0)
		dst_str[copy_size - 1] = 0;
}

int32_t iva_ram_dump_copy_mcu_sram(struct iva_dev_data *iva,
		void *dst_buf, uint32_t dst_buf_sz)
{
	const struct iva_mmr_sect_info *sect_info = &iva_sfr_mcu;

#ifdef ENABLE_SRAM_ACCESS_WORKAROUND
	iva_pmu_prepare_mcu_sram(iva);
#endif
	if (dst_buf_sz != sect_info->size) {
		dev_err(iva->dev, "%s() required size(%x) but (0x%x)\n",
				__func__, sect_info->size, dst_buf_sz);
		return -EINVAL;
	}

	iva_ram_dump_copy_section_sfrs(iva, (u32 *) dst_buf, sect_info);
	return 0;
}

int32_t iva_ram_dump_copy_iva_sfrs(struct iva_proc *proc, int shared_fd)
{
	/* TO DO: request with a specific section */
	struct iva_dev_data	*iva = proc->iva_data;
	struct iva_mem_map	*iva_map_node;
	char			*dst_buf;
	uint32_t		iva_sfr_size = (uint32_t) resource_size(iva->iva_res);
	int			ret;
	uint32_t		i;
	const struct iva_mmr_sect_info *sect_info;

	if (!iva_ctrl_is_iva_on(iva)) {
		dev_err(iva->dev, "%s() iva mcu is not booted, state(0x%lx)\n",
				__func__, proc->iva_data->state);
		return -EPERM;
	}

	iva_map_node = iva_mem_find_proc_map_with_fd(proc, (int) shared_fd);
	if (!iva_map_node) {
		dev_warn(iva->dev, "%s() fail to get iva map node with fd(%d)\n",
				__func__, shared_fd);
		return -EINVAL;
	}

	if (iva_map_node->act_size < iva_sfr_size) {
		dev_warn(iva->dev, "%s() 0x%x required but 0x%x\n",
				__func__, iva_sfr_size, iva_map_node->act_size);
		return -EINVAL;
	}

#if defined(CONFIG_SOC_EXYNOS8895)
	ret = dma_buf_begin_cpu_access(iva_map_node->dmabuf, 0, iva_sfr_size, 0);
#elif defined(CONFIG_SOC_EXYNOS9810)
	ret = dma_buf_begin_cpu_access(iva_map_node->dmabuf, 0);
#endif
	if (ret) {
		dev_warn(iva->dev, "%s() fail to begin cpu access with fd(%d)\n",
				__func__, shared_fd);
		return -ENOMEM;
	}

	dst_buf = (char *) dma_buf_kmap(iva_map_node->dmabuf, 0);
	if (!dst_buf) {
		dev_warn(iva->dev, "%s() fail to kmap ion with fd(%d)\n",
				__func__, shared_fd);
		ret = -ENOMEM;
		goto end_cpu_acc;
	}

	/* pmu */
	sect_info = &iva_sfr_pmu;
	iva_ram_dump_copy_section_sfrs(iva,
			(u32 *)(dst_buf + sect_info->base), sect_info);

	/* prepare all pmu-related preliminaries */
	iva_pmu_prepare_sfr_dump(iva);

	/* for vcm debugging */
	iva_ram_dump_copy_vcm_dbg(iva, dst_buf + VCM_DBG_SEL_SAVE_OFFSET,
			VCM_DBG_SEL_SAVE_MAX_SIZE);

	/* mcu */
	iva_ram_dump_copy_mcu_sram(iva,
			(void *)(dst_buf + IVA_VMCU_MEM_BASE_ADDR), VMCU_MEM_SIZE);

	/* general sfr section */
	for (i = 0; i < (uint32_t) ARRAY_SIZE(iva_sfr_sects); i++) {
		sect_info = iva_sfr_sects + i;
		iva_ram_dump_copy_section_sfrs(iva,
			(u32 *)(dst_buf + sect_info->base), sect_info);
	}

	dma_buf_kunmap(iva_map_node->dmabuf, 0, dst_buf);

end_cpu_acc:
#if defined(CONFIG_SOC_EXYNOS8895)
	dma_buf_end_cpu_access(iva_map_node->dmabuf, 0, IVA_CFG_SIZE, 0);
#elif defined(CONFIG_SOC_EXYNOS9810)
	dma_buf_end_cpu_access(iva_map_node->dmabuf, 0);
#endif
	return ret;
}

static bool iva_ram_dump_handler(struct iva_mbox_cb_block *ctrl_blk, uint32_t msg)
{
	struct iva_dev_data *iva = (struct iva_dev_data *) ctrl_blk->priv_data;

	dev_info(iva->dev, "%s() msg(0x%x)\n", __func__, msg);

	/* notify the error state to user */
	iva_mcu_handle_error_k(iva, mcu_err_from_irq, 31 /* SYS_ERR */);

	return true;
}

struct iva_mbox_cb_block iva_rd_cb = {
	.callback	= iva_ram_dump_handler,
	.priority	= 0,
};

int32_t iva_ram_dump_init(struct iva_dev_data *iva)
{
	iva_rd_cb.priv_data = iva;
	iva_mbox_callback_register(mbox_msg_fatal, &iva_rd_cb);

	dev_dbg(iva->dev, "%s() success!!!\n", __func__);
	return 0;
}

int32_t iva_ram_dump_deinit(struct iva_dev_data *iva)
{
	iva_mbox_callback_unregister(&iva_rd_cb);

	iva_rd_cb.priv_data = NULL;

	dev_dbg(iva->dev, "%s() success!!!\n", __func__);
	return 0;
}
