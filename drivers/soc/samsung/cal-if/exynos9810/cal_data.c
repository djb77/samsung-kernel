#include "../pmucal_common.h"
#include "../pmucal_cpu.h"
#include "../pmucal_local.h"
#include "../pmucal_rae.h"
#include "../pmucal_system.h"
#include "../pmucal_powermode.h"

#include "flexpmu_cal_cpu_exynos9810.h"
#include "flexpmu_cal_local_exynos9810.h"
#include "flexpmu_cal_p2vmap_exynos9810.h"
#include "flexpmu_cal_system_exynos9810.h"
#include "flexpmu_cal_powermode_exynos9810.h"
#include "flexpmu_cal_define_exynos9810.h"

#include "../pmucal_cp.h"
#include "pmucal_cp_exynos9810.h"

#include "cmucal-node.c"
#include "cmucal-qch.c"
#include "cmucal-sfr.c"
#include "cmucal-vclk.c"
#include "cmucal-vclklut.c"

#include "clkout_exynos9810.c"
#include "acpm_dvfs_exynos9810.h"
#include "asv_exynos9810.h"

#include "../ra.h"

#if defined(CONFIG_SEC_DEBUG)
#include <soc/samsung/exynos-pm.h>
#endif

void __iomem *cmu_base;

#define PLL_CON0_PLL_MMC	(0x100)
#define PLL_CON1_PLL_MMC	(0x104)
#define PLL_CON2_PLL_MMC	(0x108)
#define PLL_CON3_PLL_MMC	(0x10c)

#define PLL_ENABLE_SHIFT	(31)

#define MANUAL_MODE		(0x2)

#define MFR_MASK		(0xff)
#define MRR_MASK		(0x3f)
#define MFR_SHIFT		(16)
#define MRR_SHIFT		(24)
#define SSCG_EN			(16)

static int cmu_stable_done(void __iomem *cmu,
			unsigned char shift,
			unsigned int done,
			int usec)
{
	unsigned int result;

	do {
		result = get_bit(cmu, shift);

		if (result == done)
			return 0;
		udelay(1);
	} while (--usec > 0);

	return -EVCLKTIMEOUT;
}

int pll_mmc_enable(int enable)
{
	unsigned int reg;
	unsigned int cmu_mode;
	int ret;

	if (!cmu_base) {
		pr_err("%s: cmu_base cmuioremap failed\n", __func__);
		return -1;
	}

	cmu_mode = readl(cmu_base + PLL_CON2_PLL_MMC);
	writel(MANUAL_MODE, cmu_base + PLL_CON2_PLL_MMC);

	reg = readl(cmu_base + PLL_CON0_PLL_MMC);

	if (!enable) {
		reg &= ~(PLL_MUX_SEL);
		writel(reg, cmu_base + PLL_CON0_PLL_MMC);

		ret = cmu_stable_done(cmu_base + PLL_CON0_PLL_MMC, PLL_MUX_BUSY_SHIFT, 0, 100);

		if (ret)
			pr_err("pll mux change time out, \'PLL_MMC\'\n");
	}

	if (enable)
		reg |= 1 << PLL_ENABLE_SHIFT;
	else
		reg &= ~(1 << PLL_ENABLE_SHIFT);

	writel(reg, cmu_base + PLL_CON0_PLL_MMC);

	if (enable) {
		ret = cmu_stable_done(cmu_base + PLL_CON0_PLL_MMC, PLL_STABLE_SHIFT, 1, 100);
		if (ret)
			pr_err("pll time out, \'PLL_MMC\' %d\n", enable);

		reg |= PLL_MUX_SEL;
		writel(reg, cmu_base + PLL_CON0_PLL_MMC);

		ret = cmu_stable_done(cmu_base + PLL_CON0_PLL_MMC, PLL_MUX_BUSY_SHIFT, 0, 100);
		if (ret)
			pr_err("pll mux change time out, \'PLL_MMC\'\n");
	}

	writel(cmu_mode, cmu_base + PLL_CON2_PLL_MMC);

	return ret;
}

int cal_pll_mmc_check(void)
{
       unsigned int reg;
       bool ret = false;

       reg = readl(cmu_base + PLL_CON1_PLL_MMC);

       if (reg & (1 << SSCG_EN))
               ret = true;

       return ret;
}

int cal_pll_mmc_set_ssc(unsigned int mfr, unsigned int mrr, unsigned int ssc_on)
{
	unsigned int reg;
	int ret = 0;

	ret = pll_mmc_enable(0);
	if (ret) {
		pr_err("%s: pll_mmc_disable failed\n", __func__);
		return ret;
	}

	reg = readl(cmu_base + PLL_CON3_PLL_MMC);
	reg &= ~((MFR_MASK << MFR_SHIFT) | (MRR_MASK << MRR_SHIFT));

	if (ssc_on)
		reg |= ((mfr & MFR_MASK) << MFR_SHIFT) | ((mrr & MRR_MASK) << MRR_SHIFT);

	writel(reg, cmu_base + PLL_CON3_PLL_MMC);

	reg = readl(cmu_base + PLL_CON1_PLL_MMC);

	if (ssc_on)
		reg |= 1 << SSCG_EN;
	else
		reg &= ~(1 << SSCG_EN);

	writel(reg, cmu_base + PLL_CON1_PLL_MMC);
	ret = pll_mmc_enable(1);
	if (ret)
		pr_err("%s: pll_mmc_enable failed\n", __func__);

	return ret;
}

void exynos9810_cal_data_init(void)
{
	pr_info("%s: cal data init\n", __func__);

	/* cpu inform sfr initialize */
	pmucal_sys_powermode[SYS_SICD] = CPU_INFORM_SICD;
	pmucal_sys_powermode[SYS_SLEEP] = CPU_INFORM_SLEEP;
	pmucal_sys_powermode[SYS_SLEEP_USBL2] = CPU_INFORM_SLEEP_USBL2;

	cpu_inform_c2 = CPU_INFORM_C2;
	cpu_inform_cpd = CPU_INFORM_CPD;

	cmu_base = ioremap(0x1a240000, SZ_4K);
	if (!cmu_base)
		pr_err("%s: cmu_base cmuioremap failed\n", __func__);
}

void (*cal_data_init)(void) = exynos9810_cal_data_init;

#if defined(CONFIG_SEC_DEBUG)
int asv_ids_information(enum ids_info id)
{
	int res;

	switch (id) {
	case tg:
		res = asv_get_table_ver();
		break;
	case lg:
		res = asv_get_grp(dvfs_cpucl0);
		break;
	case bg:
		res = asv_get_grp(dvfs_cpucl1);
		break;
	case g3dg:
		res = asv_get_grp(dvfs_g3d);
		break;
	case mifg:
		res = asv_get_grp(dvfs_mif);
		break;
	case bids:
		res = asv_get_ids_info(dvfs_cpucl1);
		break;
	case gids:
		res = asv_get_ids_info(dvfs_g3d);
		break;
	default:
		res = 0;
		break;
	};
	return res;
}
#endif

#if defined(CONFIG_SEC_PM_DEBUG) && defined(CONFIG_DEBUG_FS)
#include <linux/debugfs.h>

#define ASV_SUMMARY_SZ	(dvfs_cp - dvfs_mif + 1)

static int asv_summary_show(struct seq_file *s, void *d)
{
	unsigned int i;
	const char *label[ASV_SUMMARY_SZ] = { "MIF", "INT", "CL0", "CL1", "G3D",
		"INTCAM", "FSYS", "CAM", "DISP", "AUD", "IVA", "SCORE", "CP" };

	seq_printf(s, "Table ver: %d\n", asv_get_table_ver());

	for (i = 0; i < ASV_SUMMARY_SZ ; i++)
		seq_printf(s, "%s: %d\n", label[i], asv_get_grp(dvfs_mif + i));

	return 0;
}

static int asv_summary_open(struct inode *inode, struct file *file)
{
	return single_open(file, asv_summary_show, inode->i_private);
}

const static struct file_operations asv_summary_fops = {
	.open		= asv_summary_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init cal_data_late_init(void)
{
	debugfs_create_file("asv_summary", 0444, NULL, NULL, &asv_summary_fops);

	return 0;
}

late_initcall(cal_data_late_init);
#endif /* CONFIG_SEC_PM_DEBUG && CONFIG_DEBUG_FS */
