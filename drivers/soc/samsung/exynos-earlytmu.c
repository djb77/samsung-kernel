/*
 * exynos_earlytmu.c - Samsung EXYNOS Ealry TMU (Thermal Management Unit)
 *
 *  Copyright (C) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <soc/samsung/exynos-earlytmu.h>
#include <soc/samsung/ect_parser.h>

#define EXYNOS_TMU_REG_TRIMINFO		0x0
#define EXYNOS_TMU_REG_CURRENT_TEMP	0x40

#define EXYNOS_TMU_CALIB_SEL_SHIFT		(23)
#define EXYNOS_TMU_CALIB_SEL_MASK		(0x1)
#define EXYNOS_TMU_TEMP_MASK			(0x1ff)
#define EXYNOS_TMU_TRIMINFO_85_SHIFT		(9)

#define TYPE_ONE_POINT_TRIMMING 0
#define TYPE_TWO_POINT_TRIMMING 1


struct exynos_earlytmu_data {
	void __iomem *base;
	int cal_type;
	int temp_error1, temp_error2;
};

static struct exynos_earlytmu_data *earlytmu_data;

/*
 * Get Frequencies by current temperature(c)
 * it implemented by bootloader temperature to protect SoC
 */
enum e_cluster_domain {
	e_cluster_none = 0,
	e_cluster_big,
	e_cluster_little,
};

int get_frequencies_by_temperature(int temperature, unsigned int *big_frequency, unsigned int *little_frequency)
{
        int no_constraint = true;
        int i, ect_temp;
        unsigned int ect_big_freq, ect_little_freq;
        struct ect_gen_param_table *boot_max_freq_table;
        void *ect_gen_block;

        *big_frequency = 0;
        *little_frequency = 0;

        ect_gen_block = ect_get_block(BLOCK_GEN_PARAM);
        if (ect_gen_block == NULL)
                return -ENOENT;

        boot_max_freq_table = ect_gen_param_get_table(ect_gen_block, "DTM_BOOT_MAX_FREQ");
        if (boot_max_freq_table == NULL)
                return -ENOENT;

        for(i = 0; i < boot_max_freq_table->num_of_row; ++i) {
                ect_temp = (int)boot_max_freq_table->parameter[i * boot_max_freq_table->num_of_col];
                ect_big_freq = boot_max_freq_table->parameter[i * boot_max_freq_table->num_of_col + e_cluster_big];
                ect_little_freq = boot_max_freq_table->parameter[i * boot_max_freq_table->num_of_col + e_cluster_little];

                if (temperature > ect_temp) {
                        *big_frequency = ect_big_freq;
                        *little_frequency = ect_little_freq;
                        no_constraint = false;
                        break;
                }
        }

        return no_constraint;
}

static int exynos_earlytmu_read(void)
{
	int code;
	int temp = 0;

	code = readl(earlytmu_data->base + EXYNOS_TMU_REG_CURRENT_TEMP) &
		EXYNOS_TMU_TEMP_MASK;

	switch (earlytmu_data->cal_type) {
	case TYPE_TWO_POINT_TRIMMING:
		temp = (code - earlytmu_data->temp_error1) *
			(85 - 25) /
			(earlytmu_data->temp_error2 - earlytmu_data->temp_error1) + 25;
		break;
	case TYPE_ONE_POINT_TRIMMING:
		temp = code - earlytmu_data->temp_error1 + 25;
		break;
	}

	if (temp > 125)	temp = 125;
	if (temp < 10)	temp = 10;

	return temp;
}

int exynos_earlytmu_get_boot_freq(int cluster)
{
	int tmu_temp;
	unsigned int big_freq, little_freq;
	int ret, freq;

	tmu_temp = exynos_earlytmu_read();

	ret = get_frequencies_by_temperature(tmu_temp, &big_freq, &little_freq);

	if (ret < 0) {
		pr_err("[EARLY TMU] fail to get frequency temp : %d\n", tmu_temp);
		return -EINVAL;
	} else if (ret == 1) {
		pr_info("[EARLY TMU] get boot frequency temp: %d, no limit\n", tmu_temp);
		return -1;
	}

	if (cluster == 0)
		freq = little_freq;
	else
		freq = big_freq;

	pr_info("[EARLY TMU] get boot frequency temp: %d, cluster: %d, freq: %d\n", tmu_temp, cluster, freq);

	return freq;
}

static int exynos_earlytmu_probe(struct platform_device *pdev)
{
	struct exynos_earlytmu_data *data;
	struct resource *res;
	unsigned int trim_info;

	data = devm_kzalloc(&pdev->dev, sizeof(struct exynos_earlytmu_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	data->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(data->base))
		return PTR_ERR(data->base);

	trim_info = readl(data->base + EXYNOS_TMU_REG_TRIMINFO);

	data->cal_type = (trim_info >> EXYNOS_TMU_CALIB_SEL_SHIFT)
			& EXYNOS_TMU_CALIB_SEL_MASK;

	data->temp_error1 = trim_info & EXYNOS_TMU_TEMP_MASK;
	data->temp_error2 = (trim_info >> EXYNOS_TMU_TRIMINFO_85_SHIFT)
				& EXYNOS_TMU_TEMP_MASK;

	earlytmu_data = data;

	return 0;
}

static int exynos_earlytmu_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id exynos_earlytmu_match[] = {
	{ .compatible = "samsung,exynos8895-earlytmu", },
	{ /* sentinel */ },
};

static struct platform_driver exynos_earlytmu_driver = {
	.driver = {
		.name   = "exynos-earlytmu",
		.owner	= THIS_MODULE,
		.of_match_table = exynos_earlytmu_match,
	},
	.probe = exynos_earlytmu_probe,
	.remove	= exynos_earlytmu_remove,
};

static int __init exynos_earlytmu_init(void)
{
	return platform_driver_register(&exynos_earlytmu_driver);
}
arch_initcall(exynos_earlytmu_init);
