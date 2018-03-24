enum acpm_dvfs_id {
	dvfs_mif = ACPM_VCLK_TYPE,
	dvfs_int,
	dvfs_cpucl0,
	dvfs_cpucl1,
	dvfs_g3d,
	dvfs_intcam,
	dvfs_cam,
	dvfs_disp,
	dvs_g3dm,
	dvs_cp,
};

struct vclk acpm_vclk_list[] = {
	CMUCAL_VCLK(dvfs_mif, NULL, NULL, NULL, NULL),
	CMUCAL_VCLK(dvfs_int, NULL, NULL, NULL, NULL),
	CMUCAL_VCLK(dvfs_cpucl0, NULL, NULL, NULL, NULL),
	CMUCAL_VCLK(dvfs_cpucl1, NULL, NULL, NULL, NULL),
	CMUCAL_VCLK(dvfs_g3d, NULL, NULL, NULL, NULL),
	CMUCAL_VCLK(dvfs_intcam, NULL, NULL, NULL, NULL),
	CMUCAL_VCLK(dvfs_cam, NULL, NULL, NULL, NULL),
	CMUCAL_VCLK(dvfs_disp, NULL, NULL, NULL, NULL),
	CMUCAL_VCLK(dvs_g3dm, NULL, NULL, NULL, NULL),
	CMUCAL_VCLK(dvs_cp, NULL, NULL, NULL, NULL),
};

unsigned int acpm_vclk_size = 10;
