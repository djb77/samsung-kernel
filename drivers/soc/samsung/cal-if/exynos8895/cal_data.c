#include "../pmucal_common.h"
#include "../pmucal_cpu.h"
#include "../pmucal_local.h"
#include "../pmucal_rae.h"
#include "../pmucal_system.h"

#include "pmucal_cpu_exynos8895.h"
#include "pmucal_local_exynos8895.h"
#include "pmucal_p2vmap_exynos8895.h"
#include "pmucal_system_exynos8895.h"

#include "cmucal-node.c"
#include "cmucal-qch.c"
#include "cmucal-sfr.c"
#include "cmucal-vclk.c"
#include "cmucal-vclklut.c"

#include "clkout_exynos8895.c"
#include "acpm_dvfs_exynos8895.h"
#include "asv_exynos8895.h"

void (*cal_data_init)(void) = NULL;

#if defined(CONFIG_SEC_FACTORY) || defined(CONFIG_SEC_DEBUG)
enum ids_info {
	tg,
	lg,
	bg,
	g3dg,
	mifg,
	bids,
	gids,
};

int asv_ids_information(enum ids_info id) {
	int res;

	switch (id) {
		case tg:
			res = asv_get_table_ver();
			break;
		case lg:
			res = asv_get_grp(dvfs_cpucl1);
			break;
		case bg:
			res = asv_get_grp(dvfs_cpucl0);
			break;
		case g3dg:
			res = asv_get_grp(dvfs_g3d);
			break;
		case mifg:
			res = asv_get_grp(dvfs_mif);
			break;
		case bids:
			res = asv_get_ids_info(dvfs_cpucl0);
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

#if defined(CONFIG_SEC_PM_DEBUG)
enum dvfs_id {
	cal_asv_dvfs_big,
	cal_asv_dvfs_little,
	cal_asv_dvfs_g3d,
	cal_asv_dvfs_mif,
	cal_asv_dvfs_int,
	cal_asv_dvfs_cam,
	cal_asv_dvfs_disp,
	cal_asv_dvs_g3dm,
	num_of_dvfs,
};

enum asv_group {
	asv_max_lv,
	dvfs_freq,
	dvfs_voltage,
	dvfs_group,
	table_group,
	num_of_asc,
};

int asv_get_information(enum dvfs_id id, enum asv_group grp, unsigned int lv) {

	int max_lv = 0, volt = 0, group = 0;

	switch (id) {
		case cal_asv_dvfs_big:
			max_lv = 0;
			volt = 0;
			group = asv_get_grp(dvfs_cpucl0);
			break;
		case cal_asv_dvfs_little:
			max_lv = 0;
			volt = 0;
			group = asv_get_grp(dvfs_cpucl1);
			break;
		case cal_asv_dvfs_g3d:
			max_lv = 0;
			volt = 0;
			group = asv_get_grp(dvfs_g3d);
			break;
		case cal_asv_dvfs_mif:
			max_lv = 0;
			volt = 0;
			group = asv_get_grp(dvfs_mif);
			break;
		case cal_asv_dvfs_int:
			max_lv = 0;
			volt = 0;
			group = asv_get_grp(dvfs_int);
			break;
		case cal_asv_dvfs_cam:
			max_lv = 0;
			volt = 0;
			group = asv_get_grp(dvfs_cam);
			break;
		case cal_asv_dvfs_disp:
			max_lv = 0;
			volt = 0;
			group = asv_get_grp(dvfs_disp);
			break;
		case cal_asv_dvs_g3dm:
			max_lv = 0;
			volt = 0;
			group = 0;
			break;
		default:
			break;
	}

	if (grp == asv_max_lv)
		return max_lv;
	else if (grp == dvfs_voltage)
		return volt;
	else if (grp == dvfs_group)
		return group;
	else
		return 0;
}
#endif /* CONFIG_SEC_PM_DEBUG */
