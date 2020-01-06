# SPDX-License-Identifier: GPL-2.0
dtb-y += exynos/exynos9810-crownlte_eur_open_17.dtb
dtb-y += exynos/exynos9810-crownlte_eur_open_18.dtb
dtb-y += exynos/exynos9810-crownlte_eur_open_19.dtb
dtb-y += exynos/exynos9810-crownlte_eur_open_21.dtb
dtb-y += exynos/exynos9810-crownlte_eur_open_23.dtb
dtb-y += exynos/exynos9810-crownlte_eur_open_24.dtb
dtb-y += exynos/exynos9810-crownlte_eur_open_25.dtb
dtb-y += exynos/exynos9810-crownlte_eur_open_26.dtb

targets += dtbs
DTB_LIST  := $(dtb-y) $(dtbo-y)
always    := $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := *.dtb*
