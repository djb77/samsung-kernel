#ifndef __ASV_EMULATOR8895_H__
#define __ASV_EMULATOR8895_H__

#include <linux/io.h>

#define ASV_TABLE_BASE	(0x10009000)
#define IDS_TABLE_BASE	(0x1000A000)

struct asv_tbl_info {
	unsigned mngs_asv_group:4;
	int mngs_modified_group:4;
	unsigned mngs_ssa1:4;
	unsigned mngs_ssa0:4;
	unsigned apollo_asv_group:4;
	int apollo_modified_group:4;
	unsigned apollo_ssa1:4;
	unsigned apollo_ssa0:4;

	unsigned g3d_asv_group:4;
	int g3d_modified_group:4;
	unsigned g3d_ssa1:4;
	unsigned g3d_ssa0:4;
	unsigned mif_asv_group:4;
	int mif_modified_group:4;
	unsigned mif_ssa1:4;
	unsigned mif_ssa0:4;

	unsigned int_asv_group:4;
	int int_modified_group:4;
	unsigned int_ssa1:4;
	unsigned int_ssa0:4;
	unsigned cam_disp_asv_group:4;
	int cam_disp_modified_group:4;
	unsigned cam_disp_ssa1:4;
	unsigned cam_disp_ssa0:4;

	unsigned asv_table_ver:7;
	unsigned fused_grp:1;
	unsigned reserved_0:8;
	unsigned cp_asv_group:4;
	int cp_modified_group:4;
	unsigned cp_ssa1:4;
	unsigned cp_ssa0:4;

	unsigned mngs_vthr:2;
	unsigned mngs_delta:2;
	unsigned apollo_vthr:2;
	unsigned apollo_delta:2;
	unsigned g3d_vthr1:2;
	unsigned g3d_vthr2:2;
	unsigned int_vthr:2;
	unsigned int_delta:2;
	unsigned mif_vthr:2;
	unsigned mif_delta:2;
};

struct ids_tbl_info {
	unsigned char ids_mngs;
	unsigned char ids_apollo;
	unsigned char ids_int;
	unsigned char ids_mif;
	unsigned char ids_g3d;
	unsigned char ids_cam_disp;
	unsigned char ids_cp;
};

static struct asv_tbl_info *asv_tbl;
static struct ids_tbl_info *ids_tbl;

int asv_get_grp(unsigned int id)
{
	int grp = -1;

	if (!asv_tbl)
		return grp;

	switch (id) {
	case dvfs_mif:
		grp = asv_tbl->mif_asv_group;
		break;
	case dvfs_int:
	case dvfs_intcam:
		grp = asv_tbl->int_asv_group;
		break;
	case dvfs_cpucl0:
		grp = asv_tbl->mngs_asv_group;
		break;
	case dvfs_cpucl1:
		grp = asv_tbl->apollo_asv_group;
		break;
	case dvfs_g3d:
		grp = asv_tbl->g3d_asv_group;
		break;
	case dvfs_cam:
	case dvfs_disp:
		grp = asv_tbl->cam_disp_asv_group;
		break;
	case dvs_cp:
		grp = asv_tbl->cp_asv_group;
		break;
	default:
		pr_info("Un-support asv grp %d\n", id);
	}

	return grp;
}

int asv_get_ids_info(unsigned int id)
{
	int grp = -1;

	if (!ids_tbl)
		return grp;

	switch (id) {
	case dvfs_mif:
		grp = ids_tbl->ids_mif;
		break;
	case dvfs_int:
	case dvfs_intcam:
		grp = ids_tbl->ids_int;
		break;
	case dvfs_cpucl0:
		grp = ids_tbl->ids_mngs;
		break;
	case dvfs_cpucl1:
		grp = ids_tbl->ids_apollo;
		break;
	case dvfs_g3d:
		grp = ids_tbl->ids_g3d;
		break;
	case dvfs_cam:
	case dvfs_disp:
		grp = ids_tbl->ids_cam_disp;
		break;
	case dvs_cp:
		grp = ids_tbl->ids_cp;
		break;
	default:
		pr_info("Un-support ids info %d\n", id);
	}

	return grp;
}

int asv_get_table_ver(void)
{
	int ver = 0;

	if (asv_tbl)
		ver = asv_tbl->asv_table_ver;

	return ver;
}

int asv_table_init(void)
{
	asv_tbl = ioremap(ASV_TABLE_BASE, SZ_4K);
	if (!asv_tbl)
		return 0;

	pr_info("asv_table_version : %d\n", asv_tbl->asv_table_ver);
	pr_info("  mngs grp : %d\n", asv_tbl->mngs_asv_group);
	pr_info("  apollo grp : %d\n", asv_tbl->apollo_asv_group);
	pr_info("  g3d grp : %d\n", asv_tbl->g3d_asv_group);
	pr_info("  mif grp : %d\n", asv_tbl->mif_asv_group);
	pr_info("  int grp : %d\n", asv_tbl->int_asv_group);
	pr_info("  cam_disp grp : %d\n", asv_tbl->cam_disp_asv_group);
	pr_info("  cp grp : %d\n", asv_tbl->cp_asv_group);

	ids_tbl = ioremap(IDS_TABLE_BASE, SZ_4K);

	return asv_tbl->asv_table_ver;
}
#endif
