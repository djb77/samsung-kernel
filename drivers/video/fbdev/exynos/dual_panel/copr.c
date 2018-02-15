/*
 * linux/drivers/video/fbdev/exynos/panel/copr.c
 *
 * Samsung Common LCD COPR Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of_gpio.h>
#include "panel.h"
#include "panel_drv.h"
#include "panel_bl.h"
#include "copr.h"

static int panel_do_copr_seqtbl_by_index(struct copr_info *copr, int index)
{
	struct panel_device *panel = to_panel_device(copr);
	struct seqinfo *tbl;
	int ret;

	if (panel == NULL) {
		panel_err("ERR:PANEL:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_warn("WARN:PANEL:%s:panel inactive state\n", __func__);
		return -EINVAL;
	}

	tbl = panel->copr.seqtbl;
	mutex_lock(&panel->op_lock);
	if (unlikely(index < 0 || index >= MAX_COPR_SEQ)) {
		panel_err("%s, invalid paramter (panel %p, index %d)\n",
				__func__, panel, index);
		ret = -EINVAL;
		goto do_exit;
	}

#ifdef DEBUG_PANEL
	pr_info("%s, %s start\n", __func__, tbl[index].name);
#endif

	ret = panel_do_seqtbl(panel, &tbl[index]);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to excute seqtbl %s\n",
				__func__, tbl->name);
		ret = -EIO;
		goto do_exit;
	}

do_exit:
	mutex_unlock(&panel->op_lock);
#ifdef DEBUG_PANEL
	pr_info("%s, %s end\n", __func__, tbl[index].name);
#endif
	return 0;
}

static int panel_set_copr(struct copr_info *copr)
{
	int ret;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	ret = panel_do_copr_seqtbl_by_index(copr, COPR_SET_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to do seqtbl\n", __func__);
		return -EIO;
	}

	if (copr->props.reg.copr_en)
		copr->props.state = COPR_REG_ON;
	else
		copr->props.state = COPR_REG_OFF;

	return 0;
}

static int panel_get_copr(struct copr_info *copr)
{
	struct panel_device *panel = to_panel_device(copr);
	struct panel_info *panel_data;
	struct timespec cur_ts, last_ts, delta_ts;
	s64 elapsed_usec;
	u8 cur_copr = 0;
	int ret;
	
	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -ENODEV;
	}
	panel_data = &panel->panel_data;
	
	if (unlikely(!copr->props.support))
		return -ENODEV;

	ktime_get_ts(&cur_ts);
	if (copr->props.state != COPR_REG_ON) {
		panel_warn("%s copr reg is not on state %d\n",
				__func__, copr->props.state);
		ret = -EINVAL;
		goto get_copr_error;
	}

	ret = panel_do_copr_seqtbl_by_index(copr, COPR_GET_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to do seqtbl\n", __func__);
		ret = -EIO;
		goto get_copr_error;
	}

	ret = resource_copy_by_name(panel_data, &cur_copr, "copr");
	if (ret < 0) {
		pr_err("%s, failed to copry copr\n", __func__);
		ret = -EIO;
		goto get_copr_error;
	}
	ktime_get_ts(&last_ts);

	delta_ts = timespec_sub(last_ts, cur_ts);
	elapsed_usec = timespec_to_ns(&delta_ts) / 1000;
	pr_debug("%s elapsed_usec %lld usec (%lld.%lld %lld.%lld)\n",
			__func__, elapsed_usec,
			timespec_to_ns(&cur_ts) / 1000000000,
			(timespec_to_ns(&cur_ts) % 1000000000) / 1000,
			timespec_to_ns(&last_ts) / 1000000000,
			(timespec_to_ns(&last_ts) % 1000000000) / 1000);

	return cur_copr;

get_copr_error:
	return ret;
}

bool copr_is_enabled(struct copr_info *copr)
{
	return (copr->props.support && copr->props.reg.copr_en && copr->props.enable);
}

static void copr_sum_update(struct copr_info *copr, int cur_copr, int cur_brt, struct timespec cur_ts)
{
	struct copr_properties *props = &copr->props;
	struct timespec delta_ts;
#ifdef DEBUG_PANEL
	static u64 copr_update_cnt;
#endif
	s64 elapsed_msec;
	int last_copr;
	int last_brt;

	if (props->last_ts.tv_sec == 0 && props->last_ts.tv_nsec == 0) {
		props->last_ts = cur_ts;
		props->last_copr = cur_copr;
		props->last_brt = cur_brt;
		props->elapsed_msec = 0;
		props->copr_sum = 0;
		props->brt_sum = 0;
		props->copr_avg = 0;
		props->brt_avg = 0;
		return;
	}

	delta_ts = timespec_sub(cur_ts, props->last_ts);
	elapsed_msec = timespec_to_ns(&delta_ts) / 1000000;

	if (elapsed_msec == 0) {
		panel_warn("%s elapsed_msec: 0 msec\n", __func__);
		return;
	}

	last_copr = props->last_copr;
	last_brt = props->last_brt;

	props->copr_sum += last_copr * elapsed_msec;
	props->brt_sum += last_brt * elapsed_msec;
	props->elapsed_msec += elapsed_msec;
	props->copr_avg = props->copr_sum / props->elapsed_msec;
	props->brt_avg = (props->brt_sum / props->elapsed_msec) / 100;

	props->total_copr_sum += last_copr * elapsed_msec;
	props->total_brt_sum += last_brt * elapsed_msec;
	props->total_elapsed_msec += elapsed_msec;
	props->total_copr_avg = props->total_copr_sum / props->total_elapsed_msec;
	props->total_brt_avg = props->total_brt_sum / props->total_elapsed_msec;
	props->last_ts = cur_ts;
	props->last_copr = cur_copr;
	props->last_brt = cur_brt;

#ifdef DEBUG_PANEL
	panel_dbg("%s copr new %d old %d avg %lld, brt new %d old %d avg %lld (copr %llu, brt %llu, %llu.%03llu sec)\n"
			"total copr avg %llu, brt avg %llu (copr %llu, brt %llu, %llu.%03lld sec) cnt %lld\n", __func__,
			cur_copr, last_copr, props->copr_avg,
			cur_brt, last_brt, props->brt_avg,
			props->copr_sum, props->brt_sum,
			props->elapsed_msec / 1000,
			props->elapsed_msec % 1000,
			props->total_copr_avg, props->total_brt_avg,
			props->total_copr_sum, props->total_brt_sum,
			props->total_elapsed_msec / 1000,
			props->total_elapsed_msec % 1000,
			++copr_update_cnt);
#endif
}

int copr_update_start(struct copr_info *copr, int count)
{
	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (atomic_read(&copr->wq.count) < count)
		atomic_set(&copr->wq.count, count);
	wake_up_interruptible_all(&copr->wq.wait);

	return 0;
}

int copr_update(struct copr_info *copr)
{
	struct panel_device *panel = to_panel_device(copr);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct copr_properties *props = &copr->props;
	struct timespec cur_ts;
	int cur_copr;
	int cur_brt;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	mutex_lock(&copr->lock);
	if (!copr_is_enabled(copr)) {
		panel_dbg("%s copr disabled\n", __func__);
		mutex_unlock(&copr->lock);
		return -EIO;
	}

	ktime_get_ts(&cur_ts);
	if (props->state == COPR_UNINITIALIZED) {
		panel_set_copr(copr);
		panel_info("%s copr register updated\n", __func__);
	}

	cur_copr = panel_get_copr(copr);
	if (cur_copr < 0 || cur_copr > 0xFF) {
		panel_err("%s failed to get copr\n", __func__);
		mutex_unlock(&copr->lock);
		return -EINVAL;
	}

	cur_brt = panel_bl->props.actual_brightness_intrp;

	copr_sum_update(copr, cur_copr, cur_brt, cur_ts);
	mutex_unlock(&copr->lock);

	return 0;
}

int copr_get_value(struct copr_info *copr)
{
	struct copr_properties *props = &copr->props;
	int cur_copr;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	mutex_lock(&copr->lock);
	if (!copr_is_enabled(copr)) {
		panel_dbg("%s copr disabled\n", __func__);
		mutex_unlock(&copr->lock);
		return -EIO;
	}

	if (props->state == COPR_UNINITIALIZED) {
		panel_set_copr(copr);
		panel_info("%s copr register updated\n", __func__);
	}

	cur_copr = panel_get_copr(copr);
	if (cur_copr < 0 || cur_copr > 0xFF) {
		panel_err("%s failed to get copr\n", __func__);
		mutex_unlock(&copr->lock);
		return -EINVAL;
	}

	mutex_unlock(&copr->lock);

	return cur_copr;
}

/**
 * copr_get_average - get copr average.
 *
 * @ copr : copr_info
 * @ c_avg : a pointer copr average to be stored.
 * @ b_avg : a pointer brt average to be stored.
 *
 * Get copr average from copr_sum and elapsed_msec.
 * And clears copr_sum and elapsed_msec to store next copr average.
 * This function returns 0 if the average is valid.
 * If not this function returns -ERRNO.
 */
int copr_get_average(struct copr_info *copr, int *c_avg, int *b_avg)
{
	struct copr_properties *props = &copr->props;
	struct timespec cur_ts;
	int copr_avg, brt_avg;
		
	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (unlikely(!c_avg || !b_avg))
		return -EINVAL;

	mutex_lock(&copr->lock);
	if (!copr_is_enabled(copr)) {
		panel_dbg("%s copr disabled\n", __func__);
		mutex_unlock(&copr->lock);
		return -EIO;
	}

	ktime_get_ts(&cur_ts);
	copr_sum_update(copr, props->last_copr, props->last_brt, cur_ts);
	copr_avg = props->copr_avg;
	brt_avg = props->brt_avg;

	panel_dbg("%s copr %d avg %lld, brt %d avg %lld (sum %llu %llu, %llu.%03llu sec) "
			"total avg %llu %llu (%llu %llu, %llu.%03lld sec)\n", __func__,
			props->last_copr, props->copr_avg,
			props->last_brt, props->brt_avg,
			props->copr_sum, props->brt_sum,
			props->elapsed_msec / 1000,
			props->elapsed_msec % 1000,
			props->total_copr_avg, props->total_brt_avg,
			props->total_copr_sum, props->total_brt_sum,
			props->total_elapsed_msec / 1000,
			props->total_elapsed_msec % 1000);

	props->elapsed_msec = 0;
	props->copr_sum = 0;
	props->brt_sum = 0;
	props->copr_avg = 0;
	props->brt_avg = 0;

	if (copr_avg < 0 || copr_avg > 0xFF) {
		panel_err("%s copr average %d invalid\n", __func__, copr_avg);
		mutex_unlock(&copr->lock);
		return -EINVAL;
	}

	*c_avg = copr_avg;
	*b_avg = brt_avg;

	mutex_unlock(&copr->lock);

	return 0;
}

static int set_spi_gpios(struct panel_device *panel, int en)
{
	int err_num = 0;
	struct copr_spi_gpios *gpio_info = &panel->spi_gpio;

	panel_info("%s en : %d\n", __func__, en);

	if (en) {
		if (gpio_direction_output(gpio_info->gpio_sck, 0))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_miso))
			goto set_exit;
		err_num++;

		if (gpio_direction_output(gpio_info->gpio_mosi, 0))
			goto set_exit;
		err_num++;

		if (gpio_direction_output(gpio_info->gpio_cs, 0))
			goto set_exit;
	} else {
		if (gpio_direction_input(gpio_info->gpio_sck))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_miso))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_mosi))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_cs))
			goto set_exit;
	}
	return 0;

set_exit:
	panel_err("%s : failed to gpio : %d\n", __func__, err_num);
	return -EIO;
}


static int get_spi_gpios_dt(struct panel_device *panel)
{
	int ret = 0;
	struct device_node *np;
	struct spi_device *spi = panel->spi;
	struct copr_spi_gpios *gpio_info = &panel->spi_gpio;
	
	if (!spi || !gpio_info) {
		panel_err("%s:spi or gpio_info is null\n", __func__);
		goto error_dt;
	}

	np = spi->master->dev.of_node;
	if (!np) {
		panel_err("%s:dev_of_node is null\n", __func__);
		goto error_dt;
	}

	gpio_info->gpio_sck = of_get_named_gpio(np, "gpio-sck", 0);
	if (gpio_info->gpio_sck < 0) {
		panel_err("%s failed to get gpio_sck from dt\n", __func__);
		goto error_dt;
	}

	gpio_info->gpio_miso = of_get_named_gpio(np, "gpio-miso", 0);
	if (gpio_info->gpio_miso < 0) {
		panel_err("%s failed to get miso from dt\n", __func__);
		goto error_dt;
	}

	gpio_info->gpio_mosi = of_get_named_gpio(np, "gpio-mosi", 0);
	if (gpio_info->gpio_mosi < 0) {
		panel_err("%s failed to get mosi from dt\n", __func__);
		goto error_dt;
	}
	
	gpio_info->gpio_cs = of_get_named_gpio(np, "cs-gpios", 0);
	if (gpio_info->gpio_cs < 0) {
		panel_err("%s failed to get cs from dt\n", __func__);
		goto error_dt;
	}

error_dt:
	return ret;
}

int copr_enable(struct copr_info *copr)
{
	struct panel_device *panel = to_panel_device(copr);
	struct panel_state *state = &panel->state;	
	struct copr_properties *props = &copr->props;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (copr_is_enabled(copr))
		return 0;
	if (set_spi_gpios(panel, 1))
		panel_err("%s:failed to set spio gpio\n", __func__);

	mutex_lock(&copr->lock);
	copr->props.enable = true;
	if (state->disp_on == PANEL_DISPLAY_ON) {
		/* TODO : check whether "copr-set" is includued in "init-seq".
		   If COPR_SET_SEQ is included in INIT_SEQ, set state COPR_REG_ON.
		   If not, copr state should be COPR_UNINITIALIZED. */
		props->state = COPR_REG_ON;
	}
	mutex_unlock(&copr->lock);
	copr_update(copr);
	panel_dbg("%s\n", __func__);

	return 0;
}

int copr_disable(struct copr_info *copr)
{
	struct panel_device *panel = to_panel_device(copr);
	
	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (!copr_is_enabled(copr))
		return 0;

	copr_update(copr);
	mutex_lock(&copr->lock);
	if (copr->props.enable) {
		copr->props.enable = false;
		copr->props.last_ts.tv_sec = 0;
		copr->props.last_ts.tv_nsec = 0;
		copr->props.last_copr = 0;
		copr->props.elapsed_msec = 0;
		copr->props.copr_sum = 0;
		copr->props.state = COPR_UNINITIALIZED;
	}
	mutex_unlock(&copr->lock);
	if (set_spi_gpios(panel, 0))
		panel_err("%s:failed to set spio gpio\n", __func__);
	panel_dbg("%s\n", __func__);

	return 0;
}

static int copr_thread(void *data)
{
	struct copr_info *copr = data;
	struct panel_device *panel = to_panel_device(copr);
	int last_copr = copr->props.last_copr;
	int last_brt = copr->props.last_brt;
	int ret;

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible(copr->wq.wait,
				(atomic_read(&copr->wq.count) > 0) &&
				(panel->state.cur_state != PANEL_STATE_ALPM) &&
				copr_is_enabled(copr));

		if (last_copr == copr->props.last_copr &&
				last_brt == copr->props.last_brt)
			atomic_dec(&copr->wq.count);
		if (!ret) {
#ifdef DEBUG_PANEL
			panel_dbg("%s copr %d brt %d count %d\n",
					__func__, last_copr, last_brt,
					atomic_read(&copr->wq.count));
#endif
			copr_update(copr);
			last_copr = copr->props.last_copr;
			last_brt = copr->props.last_brt;
			usleep_range(16660, 16670);
		}
	}

	return 0;
}

static int copr_create_thread(struct copr_info *copr)
{
	if (unlikely(!copr->props.support)) {
		panel_warn("copr unsupported\n");
		return 0;
	}

	copr->wq.thread = kthread_run(copr_thread, copr, "copr-thread");
	if (IS_ERR_OR_NULL(copr->wq.thread)) {
		panel_err("%s failed to run copr thread\n", __func__);
		copr->wq.thread = NULL;
		return PTR_ERR(copr->wq.thread);
	}

	return 0;
}

static int copr_fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct copr_info *copr;
	struct fb_event *evdata = data;
	int fb_blank;
	int early_blank;

	switch (event) {
	case FB_EVENT_BLANK:
		early_blank = 0;
		break;
	case FB_EARLY_EVENT_BLANK:
		early_blank = 1;
		break;
	default:
		return 0;
	}

	copr = container_of(self, struct copr_info, fb_notif);

	fb_blank = *(int *)evdata->data;
	pr_debug("%s: %d\n", __func__, fb_blank);

	if (evdata->info->node != 0)
		return 0;
	if (unlikely(!copr->props.support))
		return 0;

#if 0
	if (fb_blank == FB_BLANK_UNBLANK) {
		if (!early_blank)
			copr_enable(copr);
	} else if (fb_blank == FB_BLANK_POWERDOWN) {
		if (early_blank)
			copr_disable(copr);
	}
#endif

	return 0;
}

static int copr_register_fb(struct copr_info *copr)
{
	memset(&copr->fb_notif, 0, sizeof(copr->fb_notif));
	copr->fb_notif.notifier_call = copr_fb_notifier_callback;
	return fb_register_client(&copr->fb_notif);
}

int copr_probe(struct panel_device *panel, struct panel_copr_data *copr_data)
{
	struct copr_info *copr;
	int i;

	if (!panel || !copr_data) {
		pr_err("%s panel(%p) or copr_data(%p) not exist\n",
				__func__, panel, copr_data);
		return -EINVAL;
	}

	copr = &panel->copr;

	mutex_lock(&copr->lock);
	memcpy(&copr->props.reg, &copr_data->reg, sizeof(struct copr_reg));
	copr->seqtbl = copr_data->seqtbl;
	copr->nr_seqtbl = copr_data->nr_seqtbl;
	copr->maptbl = copr_data->maptbl;
	copr->nr_maptbl = copr_data->nr_maptbl;

	for (i = 0; i < copr->nr_maptbl; i++)
		copr->maptbl[i].pdata = copr;

	init_waitqueue_head(&copr->wq.wait);
	copr->props.support = true;
	copr_register_fb(copr);
	get_spi_gpios_dt(panel);

	for (i = 0; i < copr->nr_maptbl; i++)
		maptbl_init(&copr->maptbl[i]);

	if (IS_PANEL_ACTIVE(panel) &&
			copr->props.reg.copr_en) {
		copr->props.enable = true;
		copr->props.state = COPR_UNINITIALIZED;
		atomic_set(&copr->wq.count, 5);
		copr_create_thread(copr);
	}
	mutex_unlock(&copr->lock);
	pr_info("%s registered successfully\n", __func__);

	return 0;
}
