/*
 * Samsung Exynos SoC series SCORE driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SCORE_EXYNOS8895_H_
#define SCORE_EXYNOS8895_H_

enum score_pmu_reg_name {
	CLKSTOP_OPEN_CMU_SCORE_SYS_PWR_REG,
	FORCE_AUTOCLKGATE_CMU_SCORE_SYS_PWR_REG,
	CLKSTOP_CMU_SCORE_SYS_PWR_REG,
	DISABLE_PLL_CMU_SCORE_SYS_PWR_REG,
	RESET_LOGIC_SCORE_SYS_PWR_REG,
	MEMORY_SCORE_SYS_PWR_REG,
	RESET_CMU_SCORE_SYS_PWR_REG,
	LPI_RESIDUAL_CMU_SCORE_SYS_PWR_REG,
	SCORE_CONFIGURATION,
	SCORE_STATUS
};

struct score_reg score_pmu_regs[] = {
	{0x13F0, "CLKSTOP_OPEN_CMU_SCORE_SYS_PWR_REG"},
	{0x1470, "FORCE_AUTOCLKGATE_CMU_SCORE_SYS_PWR_REG"},
	{0x14B0, "CLKSTOP_CMU_SCORE_SYS_PWR_REG"},
	{0x14F0, "DISABLE_PLL_CMU_SCORE_SYS_PWR_REG"},
	{0x1530, "RESET_LOGIC_SCORE_SYS_PWR_REG"},
	{0x1570, "MEMORY_SCORE_SYS_PWR_REG"},
	{0x15B0, "RESET_CMU_SCORE_SYS_PWR_REG"},
	{0x1630, "LPI_RESIDUAL_CMU_SCORE_SYS_PWR_REG"},
	{0x4180, "SCORE_CONFIGURATION"},
	{0x4184, "SCORE_STATUS"},
};

struct score_field score_pmu_fields[] = {
	{"CLKSTOP_OPEN_CMU_SCORE_SYS_PWR_REG", 0, 2, RW},
	{"FORCE_AUTOCLKGATE_CMU_SCORE_SYS_PWR_REG", 0, 1, RW},
	{"CLKSTOP_CMU_SCORE_SYS_PWR_REG", 0, 1, RW},
	{"DISABLE_PLL_CMU_SCORE_SYS_PWR_REG", 0, 1, RW},
	{"RESET_LOGIC_SCORE_SYS_PWR_REG", 0, 2, RW},
	{"MEMORY_SCORE_SYS_PWR_REG", 0, 2, RW},
	{"RESET_CMU_SCORE_SYS_PWR_REG", 0, 2, RW},
	{"LPI_RESIDUAL_CMU_SCORE_SYS_PWR_REG", 0, 1, RW},
	{"SCORE_CONFIGURATION", 0, 4, RW},
	{"SCORE_STATUS", 0, 4, RO},
};

enum score_cmu_reg_name {
	PLL_CON0_MUX_CLKCMU_SCORE_BUS_USER,
	SCORE_CMU_CONTROLLER_OPTION,
	CLK_CON_DIV_DIV_CLK_SCORE_BUSP,
};

struct score_reg score_cmu_regs[] = {
	{0x0100, "PLL_CON0_MUX_CLKCMU_SCORE_BUS_USER"},
	{0x0800, "SCORE_CMU_CONTROLLER_OPTION"},
	{0x1800, "CLK_CON_DIV_DIV_CLK_SCORE_BUSP"},
};

struct score_field score_cmu_fields[] = {
	{"PLL_CON0_MUX_CLKCMU_SCORE_BUS_USER", 4, 1, RW},
	{"SCORE_CMU_CONTROLLER_OPTION", 28, 2, RW},
	{"CLK_CON_DIV_DIV_CLK_SCORE_BUSP", 0, 3, RW}
};

#endif
