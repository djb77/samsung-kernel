/*
 *  exynos8895_sound_vts.c
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>

#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>

static struct snd_soc_dai_link exynos8895_vts_dai_links[] = {
	{
		.name = "VTS-Trigger",
		.stream_name = "VTS-Trigger",
		.cpu_dai_name = "vts-tri",
		.platform_name = "0.vts_dma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
	},
	{
		.name = "VTS-Record",
		.stream_name = "VTS-Record",
		.cpu_dai_name = "vts-rec",
		.platform_name = "1.vts_dma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
	},
};

static struct snd_soc_dapm_route exynos8895_routes[] = {
	// sink, control, source
	{"PAD DPDM", NULL, "DMIC"},
};

static struct snd_soc_card exynos8895_vts_snd_card = {
	.name = "Exynos8895-vts-snd-card",
	.owner = THIS_MODULE,
	.dai_link = exynos8895_vts_dai_links,
	.num_links = ARRAY_SIZE(exynos8895_vts_dai_links),
	.dapm_routes = exynos8895_routes,
	.num_dapm_routes = ARRAY_SIZE(exynos8895_routes),
};

static int exynos8895_vts_sound_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct snd_soc_card *card = &exynos8895_vts_snd_card;
	int n, ret;

	dev_info(dev, "%s\n", __func__);

	card->dev = &pdev->dev;

	for (n = 0; np && n < ARRAY_SIZE(exynos8895_vts_dai_links); n++) {
		if (!exynos8895_vts_dai_links[n].cpu_dai_name) {
			exynos8895_vts_dai_links[n].cpu_of_node = of_parse_phandle(np,
					"samsung,audio-cpu", n);

			if (!exynos8895_vts_dai_links[n].cpu_of_node) {
				dev_err(&pdev->dev, "Property 'samsung,audio-cpu' missing or invalid\n");
				ret = -EINVAL;
			}
		}

		if (!exynos8895_vts_dai_links[n].platform_name) {
			exynos8895_vts_dai_links[n].platform_of_node = exynos8895_vts_dai_links[n].cpu_of_node;
		}
	}

	ret = devm_snd_soc_register_card(dev, card);

	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card() failed:%d\n", ret);

	return ret;
}

static int exynos8895_vts_sound_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	snd_soc_unregister_card(card);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id exynos8895_vts_sound_of_match[] = {
	{ .compatible = "samsung,exynos8895-vts-sound", },
	{},
};
MODULE_DEVICE_TABLE(of, exynos8895_vts_sound_of_match);
#endif /* CONFIG_OF */

static struct platform_driver exynos8895_vts_sound_driver = {
	.driver		= {
		.name	= "exynos8895-vts-sound",
		.owner	= THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = of_match_ptr(exynos8895_vts_sound_of_match),
	},
	.probe		= exynos8895_vts_sound_probe,
	.remove		= exynos8895_vts_sound_remove,
};

module_platform_driver(exynos8895_vts_sound_driver);

MODULE_DESCRIPTION("ALSA SoC EXYNOS8895 sound driver for VTS");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:exynos8895-vts-sound");

