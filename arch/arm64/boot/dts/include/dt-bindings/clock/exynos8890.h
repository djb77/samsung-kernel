/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 * Author: Andrzej Hajda <a.hajda@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants for Exynos8890 clock controller.
 */

#ifndef _DT_BINDINGS_CLOCK_EXYNOS_8890_H
#define _DT_BINDINGS_CLOCK_EXYNOS_8890_H

#define CLK_FIN_PLL    1
#define CLK_UART_BAUD0 2
#define CLK_GATE_PCLK0  207
#define CLK_GATE_PCLK1  258
#define CLK_GATE_PCLK2  259
#define CLK_GATE_PCLK3  260
#define CLK_GATE_PCLK4  261
#define CLK_GATE_PCLK5  262
#define CLK_GATE_UART0  208
#define CLK_GATE_UART1  263
#define CLK_GATE_UART2  264
#define CLK_GATE_UART3  265
#define CLK_GATE_UART4  266
#define CLK_GATE_UART5  267
#define CLK_UART0       208
#define CLK_UART1       263
#define CLK_UART2       264
#define CLK_UART3       265
#define CLK_UART4       266
#define CLK_UART5       267
#define CLK_MCT         152

#define CLK_SYSMMU_BASE 1100

#define CLK_VCLK_SYSMMU_MFC			(CLK_SYSMMU_BASE + 0)
#define CLK_VCLK_SYSMMU_MSCL			(CLK_SYSMMU_BASE + 1)
#define CLK_VCLK_SYSMMU_ISP0			(CLK_SYSMMU_BASE + 2)
#define CLK_VCLK_SYSMMU_CAM0			(CLK_SYSMMU_BASE + 3)
#define CLK_VCLK_SYSMMU_CAM1			(CLK_SYSMMU_BASE + 4)
#define CLK_VCLK_SYSMMU_AUD			(CLK_SYSMMU_BASE + 5)
#define CLK_VCLK_SYSMMU_DISP0			(CLK_SYSMMU_BASE + 6)
#define CLK_VCLK_SYSMMU_DISP1			(CLK_SYSMMU_BASE + 7)

#define CLK_GATE_SMFC	51
#endif /* _DT_BINDINGS_CLOCK_EXYNOS_8890_H */
