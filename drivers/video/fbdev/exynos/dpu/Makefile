#
# Copyright (c) 2013 Samsung Electronics Co., Ltd.
#		http://www.samsung.com
#
# Licensed under GPLv2
#

obj-$(CONFIG_EXYNOS_DPP) += dpp.o
dpp-y := dpp_drv.o
obj-$(CONFIG_EXYNOS_MIPI_DSIM) += dsim.o
dsim-y += dsim_drv.o
obj-$(CONFIG_EXYNOS_MIPI_DISPLAYPORT) += displayport.o
displayport-y += displayport_drv.o displayport_edid.o displayport_hdcp13.o
obj-$(CONFIG_SOC_EXYNOS8890) += dsim_reg_8890.o dpp_reg_8890.o decon_reg_8890.o
obj-$(CONFIG_SOC_EXYNOS8895) += dsim_reg.o dpp_reg.o displayport_reg.o decon_reg.o
obj-$(CONFIG_EXYNOS_DECON_8895) += decon.o
obj-$(CONFIG_DECON_EVENT_LOG) += event_log.o
obj-$(CONFIG_SUPPORT_DSU) += dsu_update.o
decon-y += decon_core.o decon_dsi.o decon_displayport.o decon_wb.o helper.o win_update.o bts.o
