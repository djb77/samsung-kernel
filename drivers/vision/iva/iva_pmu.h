/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_PMU_H__
#define __IVA_PMU_H__

#include "iva_ctrl.h"

enum iva_pmu_q_ch_ctrl {
	pmu_q_ch_vmcu_active	= (0x1 << 0),
	pmu_q_ch_sw_qch		= (0x1 << 1),
	pmu_q_ch_sw_qaccept_n	= (0x1 << 2),
};


enum iva_pmu_ip_id {
	pmu_csc_id	= (0x1 << 0),
	pmu_scl0_id	= (0x1 << 1),
	pmu_scl1_id	= (0x1 << 2),
	pmu_orb_id	= (0x1 << 3),
	pmu_mch_id	= (0x1 << 4),
	pmu_lmd_id	= (0x1 << 5),
	pmu_lkt_id	= (0x1 << 6),
	pmu_wig0_id	= (0x1 << 7),
	pmu_wig1_id	= (0x1 << 8),
	pmu_enf_id	= (0x1 << 9),
	pmu_vdma0_id	= (0x1 << 10),
	pmu_vdma1_id	= (0x1 << 11),
	pmu_vcf_id	= (0x1 << 12),
	pmu_vcm_id	= (0x1 << 13),
	pmu_edil_id	= (0x1 << 14),

	//----------------------------
	pmu_all_ids	= (0x1 << 15) - 1,
};

enum iva_pmu_mcu_ctrl {
	pmu_mcu_reset_n	= (0x1 << 0),
	pmu_boothold	= (0x1 << 1),
	pmu_en_sleep	= (0x1 << 2),
};

extern uint32_t iva_pmu_ctrl_q_channel(struct iva_dev_data *iva,
			enum iva_pmu_q_ch_ctrl q_ctrl, bool set);

extern uint32_t iva_pmu_reset_hwa(struct iva_dev_data *iva,
			enum iva_pmu_ip_id ip, bool release);

extern uint32_t iva_pmu_enable_clk(struct iva_dev_data *iva,
			enum iva_pmu_ip_id ip, bool enable);

/* mcu control */
extern uint32_t iva_pmu_ctrl_mcu(struct iva_dev_data *iva,
			enum iva_pmu_mcu_ctrl mcu_ctrl, bool set);
extern void	iva_pmu_prepare_mcu_sram(struct iva_dev_data *iva);
extern void	iva_pmu_reset_mcu(struct iva_dev_data *iva);

static inline void iva_pmu_reset_all(struct iva_dev_data *iva)
{
	iva_pmu_reset_hwa(iva, pmu_all_ids, false);
}

static inline void iva_pmu_release_all(struct iva_dev_data *iva)
{
	iva_pmu_reset_hwa(iva, pmu_all_ids, true);
}

static inline void iva_pmu_enable_all_clks(struct iva_dev_data *iva)
{
	iva_pmu_enable_clk(iva, pmu_all_ids, true);
}

static inline void iva_pmu_disable_all_clks(struct iva_dev_data *iva)
{
	iva_pmu_enable_clk(iva, pmu_all_ids, false);
}

extern int	iva_pmu_init(struct iva_dev_data *iva, bool en_hwa);
extern void	iva_pmu_deinit(struct iva_dev_data *iva);

extern int	iva_pmu_probe(struct iva_dev_data *iva);
extern void	iva_pmu_remove(struct iva_dev_data *iva);

#endif /* __IVA_MBOX_H */
