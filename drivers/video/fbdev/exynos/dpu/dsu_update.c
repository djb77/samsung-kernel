#include "decon.h"

static int dsu_check_info(struct decon_win_config *dsu_info, struct dsu_info_dt *dt_dsi_info) 
{
	int actual_mode;
	int width, height;

	if (unlikely(dsu_info->mode >= DSU_MODE_MAX)) {
		decon_err("DECON:ERR:%s:invalid dsu mode : %d\n",
			__func__, dsu_info->mode);
		return -EINVAL;
	}

	actual_mode = dsu_info->mode - DSU_MODE_1;

	if ((dsu_info->top != 0) || (dsu_info->left != 0)) {
		decon_err("DECON:ERR:%s:wrong dsu info : mode:%d, top:%d, left:%d",
			__func__, dsu_info->mode, dsu_info->top, dsu_info->left);
		return -EINVAL;
	}

	width = dsu_info->right - dsu_info->left;
	height = dsu_info->bottom - dsu_info->top;

	if ((dt_dsi_info->res_info[actual_mode].width != width) ||
		(dt_dsi_info->res_info[actual_mode].height != height)) {
		decon_err("DECON:ERR:%s:wrong dsu info : mode:%d, width:%d, height:%d",
			__func__, dsu_info->mode, width, height);
		return -EINVAL;
	}
	return 0;
}


int dsu_win_config(struct decon_device *decon, struct decon_win_config *windata, struct decon_reg_data *regs)
{
	int ret;
	struct dsu_info *cur_dsu;
	struct dsu_info *regs_dsu;
	struct decon_win_config *new_dsu;
	struct dsu_info_dt *dt_dsu_info = &decon->lcd_info->dt_dsu_info;

	cur_dsu = &decon->dsu;
	regs_dsu = &regs->dsu;
	new_dsu = &windata[DECON_WIN_DSU_ID];

	if (unlikely((!cur_dsu) || (!new_dsu))) {
		decon_err("DECON:ERR:%s:invalid info\n", __func__);
		return -EINVAL;
	}

	if (likely(new_dsu->mode == DSU_MODE_NONE))
		return 0;

	if (likely(new_dsu->mode == cur_dsu->mode))
		return 0;

	if (unlikely(dt_dsu_info->dsu_en == 0)) {
		decon_info("DECON_INFO:%s:this panel does not support dsu\n", __func__);
		return 0;
	}
	
	decon_info("DECON:INFO:%s:DSU Info OLD mode:%d-NEW mode:%d(%d,%d-%d,%d)\n)"
		, __func__, cur_dsu->mode, new_dsu->mode, 
		new_dsu->left, new_dsu->top, new_dsu->right, new_dsu->bottom);

	dpu_dump_winconfig(decon, windata);

	ret = dsu_check_info(new_dsu, dt_dsu_info);
	if (unlikely(ret != 0))
		return ret;

	regs_dsu->needupdate = 1;
	regs_dsu->mode = new_dsu->mode;
	regs_dsu->top = new_dsu->top;
	regs_dsu->left = new_dsu->left;
	regs_dsu->right = new_dsu->right;
	regs_dsu->bottom = new_dsu->bottom;

	return 0;
}


void dpu_dump_winconfig(struct decon_device *decon, struct decon_win_config *windata) 
{
	int i;
	struct decon_win_config *dsu_info;
	struct decon_win_config *win_info;

	dsu_info = &windata[DECON_WIN_DSU_ID];

#if 0 
	if (likely(dsu_info->mode == DSU_MODE_NONE))
		return ;

	if (likely(dsu_info->mode == decon->dsu.mode))
		return ;
#endif
	for (i = 0; i < decon->dt.max_win; i++) {
		win_info = &windata[i];
		decon_info("+HWC:%d:state:%d,dst:(%d,%d:%d,%d)\n", i, win_info->state, 
		win_info->dst.x, win_info->dst.y, win_info->dst.w, win_info->dst.h);
	}
	win_info = &windata[DECON_WIN_UPDATE_IDX];
	decon_info("+WIN:%d:state:%d,dst:(%d,%d:%d,%d)\n", i, win_info->state, 
		win_info->dst.x, win_info->dst.y, win_info->dst.w, win_info->dst.h);

	decon_info("+DSU:%d:mode:%d,dst:(%d,%d:%d,%d)\n", i, dsu_info->mode, 
		dsu_info->left, dsu_info->top, dsu_info->right, dsu_info->bottom);

}


int dpu_set_dsu_update_config(struct decon_device *decon, struct decon_reg_data *regs)
{
	int ret;
	struct decon_param param;
	struct dsu_info *cur_dsu = &regs->dsu;

	if (decon->dt.out_type != DECON_OUT_DSI)
		return 0;

	if (!cur_dsu->needupdate) 
		return 0;

	if (decon->dsu.mode == regs->dsu.mode) {
		decon_info("DECON:INFO:%s:already set dsu mode : %d\n", __func__, regs->dsu.mode);
		goto exit_config;
	}

	decon_reg_wait_idle_status_timeout(decon->id, IDLE_WAIT_TIMEOUT);

	v4l2_set_subdev_hostdata(decon->panel_sd, cur_dsu);
	ret = v4l2_subdev_call(decon->panel_sd, core, ioctl, 
		PANEL_IOC_SET_DSU, NULL);

	v4l2_set_subdev_hostdata(decon->out_sd[0], cur_dsu);
	ret = v4l2_subdev_call(decon->out_sd[0], core, ioctl,
		DSIM_IOC_SET_DSU, NULL);

	decon_to_init_param(decon, &param);

	decon_set_dsu_update(decon->id, decon->dt.dsi_mode, &param);
	
	dpu_init_win_update(decon);

	decon->dsu.mode = regs->dsu.mode;
	decon->dsu.top = regs->dsu.top;
	decon->dsu.bottom = regs->dsu.bottom;
	decon->dsu.left = regs->dsu.left;
	decon->dsu.right = regs->dsu.right;

exit_config:
	cur_dsu->needupdate = 0;
	return 0;
}

