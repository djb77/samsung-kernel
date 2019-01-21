/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "%s:%d " fmt, __func__, __LINE__

#include <linux/module.h>
#include "msm_led_flash.h"

#ifdef CONFIG_FLED_SM5703
#include <linux/leds/smfled.h>
#include <linux/leds/flashlight.h>
#endif

#define FLASH_NAME "camera-led-flash"

#undef CDBG
#define CDBG(fmt, args...) pr_debug(fmt, ##args)

extern int32_t msm_led_torch_create_classdev(
				struct platform_device *pdev, void *data);

static enum flash_type flashtype;
static struct msm_led_flash_ctrl_t fctrl;
#ifdef CONFIG_LEDS_SM5705
extern int sm5705_fled_led_off(unsigned char index);
extern int sm5705_fled_torch_on(unsigned char index);
extern int sm5705_fled_flash_on(unsigned char index);
extern int sm5705_fled_preflash(unsigned char index);
#endif

#ifdef CONFIG_FLED_SM5703
extern int sm5703_fled_led_off(sm_fled_info_t *fled_info);
extern int sm5703_fled_torch_on(sm_fled_info_t *fled_info);
extern int sm5703_fled_flash_on(sm_fled_info_t *fled_info);
#endif

#if defined(CONFIG_DUAL_LEDS_FLASH)
extern int sm5705_fled_pre_flash_on(unsigned char index, int32_t pre_flash_current_mA);
extern int sm5705_fled_flash_on_set_current(unsigned char index, int32_t flash_current_mA);
#endif

#ifdef CONFIG_LEDS_S2MU005
extern int ss_rear_flash_led_flash_on(void);
extern int ss_rear_flash_led_torch_on(void);
extern int ss_rear_flash_led_turn_off(void);
extern int ss_rear_torch_set_flashlight(bool isFlashlight);
#endif

static int32_t msm_led_trigger_get_subdev_id(struct msm_led_flash_ctrl_t *fctrl,
	void *arg)
{
	uint32_t *subdev_id = (uint32_t *)arg;
	if (!subdev_id) {
		pr_err("%s:%d failed\n", __func__, __LINE__);
		return -EINVAL;
	}
	*subdev_id = fctrl->pdev->id;
	CDBG("%s:%d subdev_id %d\n", __func__, __LINE__, *subdev_id);
	return 0;
}

int msm_led_release(struct msm_led_flash_ctrl_t *fctrl)
{
	//Added release function to resolve panic issue
	return 0;
}

#ifdef CONFIG_LEDS_S2MU005
static int32_t msm_led_trigger_config(struct msm_led_flash_ctrl_t *fctrl,
	void *data)
{
	int rc = 0;
	struct msm_camera_led_cfg_t *cfg = (struct msm_camera_led_cfg_t *)data;

	if (!fctrl) {
		pr_err("[%s:%d]failed\n", __func__, __LINE__);
		return -EINVAL;
	}
	
	CDBG("cfg->cfgtype = %d\n", cfg->cfgtype);
	switch (cfg->cfgtype) {
		case MSM_CAMERA_LED_OFF:
			CDBG("MSM_CAMERA_LED_OFF");
			ss_rear_flash_led_turn_off();
		break;
		case MSM_CAMERA_LED_LOW:
			ss_rear_torch_set_flashlight(false);
			ss_rear_flash_led_torch_on();
		break;
		case MSM_CAMERA_LED_TORCH:
			ss_rear_torch_set_flashlight(true);
			ss_rear_flash_led_torch_on();
		break;
		case MSM_CAMERA_LED_HIGH:
			ss_rear_flash_led_flash_on();
		break;
		case MSM_CAMERA_LED_INIT:
		case MSM_CAMERA_LED_RELEASE:
			ss_rear_flash_led_turn_off();
		break;
		default:
		break;
	}
	return rc;
}

#elif defined CONFIG_LEDS_SM5705
static int32_t msm_led_trigger_config(struct msm_led_flash_ctrl_t *fctrl,
	void *data)
{
	int rc = 0;
	struct msm_camera_led_cfg_t *cfg = (struct msm_camera_led_cfg_t *)data;

	if (!fctrl) {
		pr_err("[%s:%d]failed\n", __func__, __LINE__);
		return -EINVAL;
	}
	switch (cfg->cfgtype) {
		case MSM_CAMERA_LED_OFF:
			sm5705_fled_led_off(0);
#if defined(CONFIG_DUAL_LEDS_FLASH)
			sm5705_fled_led_off(1);
#endif
		break;
		case MSM_CAMERA_LED_LOW:
#if defined(CONFIG_DUAL_LEDS_FLASH)
			CDBG("[%s:%d]MSM_CAMERA_LED_LOW cfg->flash_current[0] = %d \n", __func__, __LINE__, cfg->flash_current[0]);
			CDBG("[%s:%d]MSM_CAMERA_LED_LOW cfg->flash_current[1] = %d \n", __func__, __LINE__, cfg->flash_current[1]);
			if(cfg->flash_current[0] > 0) {
				sm5705_fled_pre_flash_on(0,cfg->flash_current[0]);
			} else {
			sm5705_fled_torch_on(0);
			}

			if(cfg->flash_current[1] > 0) {
				sm5705_fled_pre_flash_on(1,cfg->flash_current[1]);
			} else {
			sm5705_fled_torch_on(1);
			}
#elif defined(CONFIG_LEDS_SEPARATE_PREFLASH)
			sm5705_fled_preflash(0);
#else
			sm5705_fled_torch_on(0);
#endif
		break;
		case MSM_CAMERA_LED_HIGH:
			sm5705_fled_led_off(0);
#if defined(CONFIG_DUAL_LEDS_FLASH)
			sm5705_fled_led_off(1);
			CDBG("[%s:%d]MSM_CAMERA_LED_HIGH cfg->flash_current[0] = %d \n", __func__, __LINE__, cfg->flash_current[0]);
			CDBG("[%s:%d]MSM_CAMERA_LED_HIGH cfg->flash_current[1] = %d \n", __func__, __LINE__, cfg->flash_current[1]);
			if(cfg->flash_current[0] > 0) {
				sm5705_fled_flash_on_set_current(0, cfg->flash_current[0]);
			} else {
			sm5705_fled_flash_on(0);
			}

			if(cfg->flash_current[1] > 0) {
				sm5705_fled_flash_on_set_current(1, cfg->flash_current[1]);
			} else {
			sm5705_fled_flash_on(1);
			}
#else
			sm5705_fled_flash_on(0);
#endif
		break;
		case MSM_CAMERA_LED_TORCH:
			sm5705_fled_torch_on(0);
		break;
		case MSM_CAMERA_LED_INIT:
		case MSM_CAMERA_LED_RELEASE:
			sm5705_fled_led_off(0);
#if defined(CONFIG_DUAL_LEDS_FLASH)
			sm5705_fled_led_off(1);
#endif
		break;
		default:
		break;
	}
	return rc;
}

#elif defined CONFIG_FLED_SM5703
static int32_t msm_led_trigger_config(struct msm_led_flash_ctrl_t *fctrl,
	void *data)
{       
	int rc = 0;
	struct msm_camera_led_cfg_t *cfg = (struct msm_camera_led_cfg_t *)data;
        sm_fled_info_t *fled_info = sm_fled_get_info_by_name(NULL);
	if (!fctrl) {
		pr_err("[%s:%d]failed\n", __func__, __LINE__);
		return -EINVAL;
	}
	switch (cfg->cfgtype) {
		case MSM_CAMERA_LED_OFF:
			sm5703_fled_led_off(fled_info);
		break;
		case MSM_CAMERA_LED_LOW:
			sm5703_fled_torch_on(fled_info);
		break;
		case MSM_CAMERA_LED_HIGH:
			sm5703_fled_led_off(fled_info);
			sm5703_fled_flash_on(fled_info);
		break;
		case MSM_CAMERA_LED_INIT:
		case MSM_CAMERA_LED_RELEASE:
			sm5703_fled_led_off(fled_info);
		break;
		default:
		break;
	}
	return rc;
}

#else


static int32_t msm_led_trigger_config(struct msm_led_flash_ctrl_t *fctrl,
	void *data)
{
	int rc = 0;
	struct msm_camera_led_cfg_t *cfg = (struct msm_camera_led_cfg_t *)data;
	uint32_t i;
	uint32_t curr_l, max_curr_l;
	CDBG("called led_state %d\n", cfg->cfgtype);

	if (!fctrl) {
		pr_err("failed\n");
		return -EINVAL;
	}

	switch (cfg->cfgtype) {
	case MSM_CAMERA_LED_OFF:
		/* Flash off */
		for (i = 0; i < fctrl->flash_num_sources; i++)
			if (fctrl->flash_trigger[i])
				led_trigger_event(fctrl->flash_trigger[i], 0);
		/* Torch off */
		for (i = 0; i < fctrl->torch_num_sources; i++)
			if (fctrl->torch_trigger[i])
				led_trigger_event(fctrl->torch_trigger[i], 0);
		break;

	case MSM_CAMERA_LED_LOW:
		for (i = 0; i < fctrl->torch_num_sources; i++)
			if (fctrl->torch_trigger[i]) {
				max_curr_l = fctrl->torch_max_current[i];
				if (cfg->torch_current[i] >= 0 &&
					cfg->torch_current[i] < max_curr_l) {
					curr_l = cfg->torch_current[i];
				} else {
					curr_l = fctrl->torch_op_current[i];
					pr_debug("LED torch %d clamped %d\n",
						i, curr_l);
				}
				led_trigger_event(fctrl->torch_trigger[i],
						curr_l);
			}
		break;

	case MSM_CAMERA_LED_HIGH:
		/* Torch off */
		for (i = 0; i < fctrl->torch_num_sources; i++)
			if (fctrl->torch_trigger[i])
				led_trigger_event(fctrl->torch_trigger[i], 0);

		for (i = 0; i < fctrl->flash_num_sources; i++)
			if (fctrl->flash_trigger[i]) {
				max_curr_l = fctrl->flash_max_current[i];
				if (cfg->flash_current[i] >= 0 &&
					cfg->flash_current[i] < max_curr_l) {
					curr_l = cfg->flash_current[i];
				} else {
					curr_l = fctrl->flash_op_current[i];
					pr_debug("LED flash %d clamped %d\n",
						i, curr_l);
				}
				led_trigger_event(fctrl->flash_trigger[i],
					curr_l);
			}
		break;

	case MSM_CAMERA_LED_INIT:
	case MSM_CAMERA_LED_RELEASE:
		/* Flash off */
		for (i = 0; i < fctrl->flash_num_sources; i++)
			if (fctrl->flash_trigger[i])
				led_trigger_event(fctrl->flash_trigger[i], 0);
		/* Torch off */
		for (i = 0; i < fctrl->torch_num_sources; i++)
			if (fctrl->torch_trigger[i])
				led_trigger_event(fctrl->torch_trigger[i], 0);
		break;

	default:
		rc = -EFAULT;
		break;
	}
	CDBG("flash_set_led_state: return %d\n", rc);
	return rc;
}
#endif
static const struct of_device_id msm_led_trigger_dt_match[] = {
	{.compatible = "qcom,camera-led-flash"},
	{}
};

MODULE_DEVICE_TABLE(of, msm_led_trigger_dt_match);

static struct platform_driver msm_led_trigger_driver = {
	.driver = {
		.name = FLASH_NAME,
		.owner = THIS_MODULE,
		.of_match_table = msm_led_trigger_dt_match,
	},
};

static int32_t msm_led_trigger_probe(struct platform_device *pdev)
{
	int32_t rc = 0, rc_1 = 0, i = 0;
	struct device_node *of_node = pdev->dev.of_node;
	struct device_node *flash_src_node = NULL;
	uint32_t count = 0;
	struct led_trigger *temp = NULL;

	CDBG("called\n");

	if (!of_node) {
		pr_err("of_node NULL\n");
		return -EINVAL;
	}

	fctrl.pdev = pdev;
	fctrl.flash_num_sources = 0;
	fctrl.torch_num_sources = 0;

	rc = of_property_read_u32(of_node, "cell-index", &pdev->id);
	if (rc < 0) {
		pr_err("failed\n");
		return -EINVAL;
	}
	CDBG("pdev id %d\n", pdev->id);

	rc = of_property_read_u32(of_node,
			"qcom,flash-type", &flashtype);
	if (rc < 0) {
		pr_err("flash-type: read failed\n");
		return -EINVAL;
	}

	/* Flash source */
	if (of_get_property(of_node, "qcom,flash-source", &count)) {
		count /= sizeof(uint32_t);
		CDBG("qcom,flash-source count %d\n", count);
		if (count > MAX_LED_TRIGGERS) {
			pr_err("invalid count qcom,flash-source %d\n", count);
			return -EINVAL;
		}
		fctrl.flash_num_sources = count;
		for (i = 0; i < fctrl.flash_num_sources; i++) {
			flash_src_node = of_parse_phandle(of_node,
				"qcom,flash-source", i);
			if (!flash_src_node) {
				pr_err("flash_src_node %d NULL\n", i);
				continue;
			}

			rc = of_property_read_string(flash_src_node,
				"linux,default-trigger",
				&fctrl.flash_trigger_name[i]);

			rc_1 = of_property_read_string(flash_src_node,
				"qcom,default-led-trigger",
				&fctrl.flash_trigger_name[i]);
			if ((rc < 0) && (rc_1 < 0)) {
				pr_err("default-trigger: read failed\n");
				of_node_put(flash_src_node);
				continue;
			}

			CDBG("default trigger %s\n",
				fctrl.flash_trigger_name[i]);

			if (flashtype == GPIO_FLASH) {
				/* use fake current */
				fctrl.flash_op_current[i] = LED_FULL;
			} else {
				rc = of_property_read_u32(flash_src_node,
					"qcom,current",
					&fctrl.flash_op_current[i]);
				rc_1 = of_property_read_u32(flash_src_node,
					"qcom,max-current",
					&fctrl.flash_max_current[i]);
				if ((rc < 0) || (rc_1 < 0)) {
					pr_err("current: read failed\n");
					of_node_put(flash_src_node);
					continue;
				}
			}

			of_node_put(flash_src_node);

			CDBG("max_current[%d] %d\n",
				i, fctrl.flash_op_current[i]);

			led_trigger_register_simple(fctrl.flash_trigger_name[i],
				&fctrl.flash_trigger[i]);

			if (flashtype == GPIO_FLASH)
				if (fctrl.flash_trigger[i])
					temp = fctrl.flash_trigger[i];
		}

	}
	/* Torch source */
	if (of_get_property(of_node, "qcom,torch-source", &count)) {
		count /= sizeof(uint32_t);
		CDBG("qcom,torch-source count %d\n", count);
		if (count > MAX_LED_TRIGGERS) {
			pr_err("invalid count qcom,torch-source %d\n", count);
			return -EINVAL;
		}
		fctrl.torch_num_sources = count;

		for (i = 0; i < fctrl.torch_num_sources; i++) {
			flash_src_node = of_parse_phandle(of_node,
				"qcom,torch-source", i);
			if (!flash_src_node) {
				pr_err("torch_src_node %d NULL\n", i);
				continue;
			}

			rc = of_property_read_string(flash_src_node,
				"linux,default-trigger",
				&fctrl.torch_trigger_name[i]);

			rc_1 = of_property_read_string(flash_src_node,
				"qcom,default-led-trigger",
				&fctrl.torch_trigger_name[i]);
			if ((rc < 0) && (rc_1 < 0)) {
				pr_err("default-trigger: read failed\n");
				of_node_put(flash_src_node);
				continue;
			}

			CDBG("default trigger %s\n",
				fctrl.torch_trigger_name[i]);

			if (flashtype == GPIO_FLASH) {
				/* use fake current */
				fctrl.torch_op_current[i] = LED_HALF;
			} else {
				rc = of_property_read_u32(flash_src_node,
					"qcom,current",
					&fctrl.torch_op_current[i]);
				rc_1 = of_property_read_u32(flash_src_node,
					"qcom,max-current",
					&fctrl.torch_max_current[i]);
				if ((rc < 0) || (rc_1 < 0)) {
					pr_err("current: read failed\n");
					of_node_put(flash_src_node);
					continue;
				}
			}

			of_node_put(flash_src_node);

			CDBG("torch max_current[%d] %d\n",
				i, fctrl.torch_op_current[i]);

			led_trigger_register_simple(fctrl.torch_trigger_name[i],
				&fctrl.torch_trigger[i]);

			if (flashtype == GPIO_FLASH)
				if (temp && !fctrl.torch_trigger[i])
					fctrl.torch_trigger[i] = temp;
		}
	}

	rc = msm_led_flash_create_v4lsubdev(pdev, &fctrl);
	if (!rc)
		msm_led_torch_create_classdev(pdev, &fctrl);

	return rc;
}

static int __init msm_led_trigger_add_driver(void)
{
	CDBG("called\n");
	return platform_driver_probe(&msm_led_trigger_driver,
		msm_led_trigger_probe);
}

static struct msm_flash_fn_t msm_led_trigger_func_tbl = {
	.flash_get_subdev_id = msm_led_trigger_get_subdev_id,
	.flash_led_config = msm_led_trigger_config,
	.flash_led_release = msm_led_release,
};

static struct msm_led_flash_ctrl_t fctrl = {
	.func_tbl = &msm_led_trigger_func_tbl,
};

module_init(msm_led_trigger_add_driver);
MODULE_DESCRIPTION("LED TRIGGER FLASH");
MODULE_LICENSE("GPL v2");
