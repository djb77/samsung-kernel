/*
 *  driver/misc/sec_mux_sel.c
 *  Samsung Mux Selection Driver
 *
 *  Copyright (C) 2017 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/sec_mux_sel.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/wakelock.h>

struct sec_mux_sel_info *mux_sel;

int EAR_ADC_MUX_SEL_NUM;
int BATT_ID_MUX_SEL_NUM;
int BATT_THM_MUX_SEL_NUM;
int CHG_THM_MUX_SEL_NUM;
int AP_THM_MUX_SEL_NUM;
int WPC_THM_MUX_SEL_NUM;
int SLAVE_CHG_THM_MUX_SEL_NUM;
int BLKT_THM_MUX_SEL_NUM;

/*
* below data tree description has to be defined in dtsi file 
* mux_sel {
* 	compatible = "samsung,sec-mux-sel";
*	mux_sel,mux_sel_1_en = <1>; // enable to use
*	mux_sel,mux_sel_1 = <&tlmm 7 0x0>; // gpio number of mux_sel
*	mux_sel,mux_sel_1_type = <16>; // BATT_THM_MUX_SEL | AP_THM_MUX_SEL, two thermistor outputs 
*	mux_sel,mux_sel_1_mpp = <4>; // MPP_4, the number of MPP
*	mux_sel,mux_sel_1_low = <2>; // SEC_MUX_SEL_BATT_THM, selection of mux_sel_1_en low
*	mux_sel,mux_sel_1_high = <4>; // SEC_MUX_SEL_AP_THM, selection of mux_sel_1_en high
};
*/

/*
* int mux_sel : name of mux_sel
* int adc_type : refer to eum value of sec_mux_sel_type
* int mutex_on : mutex on/off value 1 or 0
* usage : sec_mpp_mux_control(BATT_THM_MUX_SEL_NUM, SEC_MUX_SEL_BATT_THM, 1); // mutex lock
*         sec_mpp_mux_control(BATT_THM_MUX_SEL_NUM, SEC_MUX_SEL_BATT_THM, 0); // mutex unlock
*/
void sec_mpp_mux_control(int mux_sel_n, int adc_type, int mutex_on)
{
	int mux_sel_level = 0;
	
	pr_debug("%s mux_sel_n = %d,  adc_type = %d, mutex_on = %d \n", __func__, mux_sel_n, adc_type, mutex_on);

	if(mux_sel_n == 0) {
		pr_err("%s this mux sel number dose not exist \n" , __func__);
		return;	
	}

	if(mux_sel == NULL) {
		pr_err("%s driver is not probed \n" , __func__);
		return;
	}

	if (mutex_on) {
		mutex_lock(&mux_sel->mpp_share_mutex);

		switch(mux_sel_n) {
			case SEC_MUX_SEL_1:
				if(mux_sel->pdata->mux_sel_1_en) {
					if(adc_type == mux_sel->pdata->mux_sel_1_low)
						mux_sel_level = 0;
					else if(adc_type == mux_sel->pdata->mux_sel_1_high)
						mux_sel_level = 1;
					else {
						pr_info("%s wrong adc_type (%d)!! \n", __func__, adc_type);
						break;
					}
					gpio_direction_output(mux_sel->pdata->mux_sel_1, mux_sel_level);
					//pr_info("%s: [mux_sel_1] level : %d(%d), mutex status : %d\n", __func__, mux_sel_level, gpio_get_value(mux_sel->pdata->mux_sel_1), mutex_on);					
				}
				break;
			case SEC_MUX_SEL_2:
				if(mux_sel->pdata->mux_sel_2_en) {
					if(adc_type == mux_sel->pdata->mux_sel_2_low)
						mux_sel_level = 0;
					else if(adc_type == mux_sel->pdata->mux_sel_2_high)
						mux_sel_level = 1;
					else {
						pr_info("%s wrong adc_type (%d)!! \n", __func__, adc_type);
						break;
					}
					gpio_direction_output(mux_sel->pdata->mux_sel_2, mux_sel_level);
					//pr_info("%s: [mux_sel_2] level : %d(%d), mutex status : %d\n", __func__, mux_sel_level, gpio_get_value(mux_sel->pdata->mux_sel_2), mutex_on);					
				}
				break;
			case SEC_MUX_SEL_3:
				if(mux_sel->pdata->mux_sel_3_en) {
					if(adc_type == mux_sel->pdata->mux_sel_3_low)
						mux_sel_level = 0;
					else if(adc_type == mux_sel->pdata->mux_sel_3_high)
						mux_sel_level = 1;
					else {
						pr_info("%s wrong adc_type (%d)!! \n", __func__, adc_type);
						break;
					}
					gpio_direction_output(mux_sel->pdata->mux_sel_3, mux_sel_level);
					//pr_info("%s: [mux_sel_3] level : %d(%d), mutex status : %d\n", __func__, mux_sel_level, gpio_get_value(mux_sel->pdata->mux_sel_3), mutex_on);					
				}
				break;
			default:
				dev_err(mux_sel->dev,
					"%s: Invalid mux sel\n", __func__);
				break;				
		}
	} else {
		pr_debug("%s: MPP mux_sel : %d, ml : %d\n", __func__, mux_sel_level, mutex_on);
		mutex_unlock(&mux_sel->mpp_share_mutex);
	}
}
EXPORT_SYMBOL_GPL(sec_mpp_mux_control);

static int sec_mux_sel_parse_dt(struct device *dev,
		struct sec_mux_sel_info *mux_sel)
{
	struct device_node *np = of_find_node_by_name(NULL, "mux_sel");
	sec_mux_sel_platform_data_t *pdata = mux_sel->pdata;
	int ret;

	if (!np) {
		pr_info("%s: np NULL\n", __func__);
		return 1;
	}

	pdata->mux_sel_1_en = of_property_read_bool(np,
		"mux_sel,mux_sel_1_en");
	pr_info("%s: mux_sel_1_en: %d \n", __func__, pdata->mux_sel_1_en);

	if(pdata->mux_sel_1_en) {
		pdata->mux_sel_1 = of_get_named_gpio(np, "mux_sel,mux_sel_1", 0);
		pr_info("%s : mux_sel_1 %d \n", __func__,pdata->mux_sel_1);
		if (pdata->mux_sel_1 < 0)
			pdata->mux_sel_1 = 0;

		ret = of_property_read_u32(np, "mux_sel,mux_sel_1_type",
			&pdata->mux_sel_1_type);
		if (ret)
			pr_info("%s: mux_sel_1_type is Empty\n", __func__);

		ret = of_property_read_u32(np, "mux_sel,mux_sel_1_mpp",
			&pdata->mux_sel_1_mpp);
		if (ret)
			pr_info("%s: mux_sel_1_mpp is Empty\n", __func__);

		ret = of_property_read_u32(np, "mux_sel,mux_sel_1_low",
			&pdata->mux_sel_1_low);
		if (ret)
			pr_info("%s: mux_sel_1_low is Empty\n", __func__);

		ret = of_property_read_u32(np, "mux_sel,mux_sel_1_high",
			&pdata->mux_sel_1_high);
		if (ret)
			pr_info("%s: mux_sel_1_high is Empty\n", __func__);

		pr_info("%s mux_sel_1 = %d, pdata->mux_sel_1_type = %d, mux_sel_1_mpp = %d, mux_sel_1_low = %d, mux_sel_1_high = %d \n",
			__func__, pdata->mux_sel_1, pdata->mux_sel_1_type, pdata->mux_sel_1_mpp, pdata->mux_sel_1_low, pdata->mux_sel_1_high);
	}

	pdata->mux_sel_2_en = of_property_read_bool(np,
		"mux_sel,mux_sel_2_en");
	pr_info("%s: mux_sel_2_en: %d \n", __func__, pdata->mux_sel_2_en);

	if(pdata->mux_sel_2_en) {	
		pdata->mux_sel_2 = of_get_named_gpio(np, "mux_sel,mux_sel_2", 0);
		pr_info("%s : mux_sel_2 %d \n", __func__,pdata->mux_sel_2);
		if (pdata->mux_sel_2 < 0)
			pdata->mux_sel_2 = 0;

		ret = of_property_read_u32(np, "mux_sel,mux_sel_2_type",
			&pdata->mux_sel_2_type);
		if (ret)
			pr_info("%s: mux_sel_2_type is Empty\n", __func__);

		ret = of_property_read_u32(np, "mux_sel,mux_sel_2_mpp",
			&pdata->mux_sel_2_mpp);
		if (ret)
			pr_info("%s: mux_sel_2_mpp is Empty\n", __func__);

		ret = of_property_read_u32(np, "mux_sel,mux_sel_2_low",
			&pdata->mux_sel_2_low);
		if (ret)
			pr_info("%s: mux_sel_2_low is Empty\n", __func__);

		ret = of_property_read_u32(np, "mux_sel,mux_sel_2_high",
			&pdata->mux_sel_2_high);
		if (ret)
			pr_info("%s: mux_sel_2_high is Empty\n", __func__);

		pr_info("%s mux_sel_2 = %d, pdata->mux_sel_2_type = %d, mux_sel_2_mpp = %d, mux_sel_2_low = %d, mux_sel_2_high = %d \n",
			__func__, pdata->mux_sel_2, pdata->mux_sel_2_type, pdata->mux_sel_2_mpp, pdata->mux_sel_2_low, pdata->mux_sel_2_high);		
	}

	pdata->mux_sel_3_en = of_property_read_bool(np,
		"mux_sel,mux_sel_3_en");
	pr_info("%s: mux_sel_3_en: %d \n", __func__, pdata->mux_sel_3_en);

	if(pdata->mux_sel_3_en) {	
		pdata->mux_sel_3 = of_get_named_gpio(np, "mux_sel,mux_sel_3", 0);
		pr_info("%s : mux_sel_3 %d \n", __func__,pdata->mux_sel_3);
		if (pdata->mux_sel_3 < 0)
			pdata->mux_sel_3 = 0;

		ret = of_property_read_u32(np, "mux_sel,mux_sel_3_type",
			&pdata->mux_sel_3_type);
		if (ret)
			pr_info("%s: mux_sel_3_type is Empty\n", __func__);

		ret = of_property_read_u32(np, "mux_sel,mux_sel_3_mpp",
			&pdata->mux_sel_3_mpp);
		if (ret)
			pr_info("%s: mux_sel_3_mpp is Empty\n", __func__);

		ret = of_property_read_u32(np, "mux_sel,mux_sel_3_low",
			&pdata->mux_sel_3_low);
		if (ret)
			pr_info("%s: mux_sel_3_low is Empty\n", __func__);

		ret = of_property_read_u32(np, "mux_sel,mux_sel_3_high",
			&pdata->mux_sel_3_high);
		if (ret)
			pr_info("%s: mux_sel_3_high is Empty\n", __func__);

		pr_info("%s mux_sel_3 = %d, pdata->mux_sel_3_type = %d, mux_sel_3_mpp = %d, mux_sel_3_low = %d, mux_sel_3_high = %d \n",
			__func__, pdata->mux_sel_3, pdata->mux_sel_3_type, pdata->mux_sel_3_mpp, pdata->mux_sel_3_low, pdata->mux_sel_3_high);		
	}

	pr_info("%s: dt load done \n", __func__);

	return 0;
}

static void sec_mux_sel_get_info(void)
{
	int ret = 0;

	EAR_ADC_MUX_SEL_NUM = 0;
	BATT_ID_MUX_SEL_NUM = 0;
	BATT_THM_MUX_SEL_NUM = 0;
	CHG_THM_MUX_SEL_NUM = 0;
	AP_THM_MUX_SEL_NUM = 0;
	WPC_THM_MUX_SEL_NUM = 0;
	SLAVE_CHG_THM_MUX_SEL_NUM = 0;
	BLKT_THM_MUX_SEL_NUM = 0;

	pr_info("%s mux_sel_1_type = %d, mux_sel_2_type = %d \n", __func__, mux_sel->pdata->mux_sel_1_type, mux_sel->pdata->mux_sel_2_type);

	if (mux_sel->pdata->mux_sel_1_en) {
		ret = gpio_request(mux_sel->pdata->mux_sel_1, "mux-sel");
		if (ret) {
			pr_err("failed to request GPIO %u\n", mux_sel->pdata->mux_sel_1);
		}

		if (mux_sel->pdata->mux_sel_1_type & EAR_ADC_MUX_SEL)
			EAR_ADC_MUX_SEL_NUM = SEC_MUX_SEL_1;
		if (mux_sel->pdata->mux_sel_1_type & BATT_ID_MUX_SEL)
			BATT_ID_MUX_SEL_NUM = SEC_MUX_SEL_1;
		if (mux_sel->pdata->mux_sel_1_type & BATT_THM_MUX_SEL)
			BATT_THM_MUX_SEL_NUM = SEC_MUX_SEL_1;
		if (mux_sel->pdata->mux_sel_1_type & CHG_THM_MUX_SEL)
			CHG_THM_MUX_SEL_NUM = SEC_MUX_SEL_1;
		if (mux_sel->pdata->mux_sel_1_type & SLAVE_CHG_THM_MUX_SEL)
			SLAVE_CHG_THM_MUX_SEL_NUM = SEC_MUX_SEL_1;
		if (mux_sel->pdata->mux_sel_1_type & AP_THM_MUX_SEL)
			AP_THM_MUX_SEL_NUM = SEC_MUX_SEL_1;
		if (mux_sel->pdata->mux_sel_1_type & WPC_THM_MUX_SEL)
			WPC_THM_MUX_SEL_NUM = SEC_MUX_SEL_1;
		if (mux_sel->pdata->mux_sel_1_type & BLKT_THM_MUX_SEL)
			BLKT_THM_MUX_SEL_NUM = SEC_MUX_SEL_1;
	}

	if (mux_sel->pdata->mux_sel_2_en) {
		ret = gpio_request(mux_sel->pdata->mux_sel_2, "mux-sel2");
		if (ret) {
			pr_err("failed to request GPIO %u\n", mux_sel->pdata->mux_sel_2);
		}

		if (mux_sel->pdata->mux_sel_2_type & EAR_ADC_MUX_SEL)
			EAR_ADC_MUX_SEL_NUM = SEC_MUX_SEL_2;
		if (mux_sel->pdata->mux_sel_2_type & BATT_ID_MUX_SEL)
			BATT_ID_MUX_SEL_NUM = SEC_MUX_SEL_2;
		if (mux_sel->pdata->mux_sel_2_type & BATT_THM_MUX_SEL)
			BATT_THM_MUX_SEL_NUM = SEC_MUX_SEL_2;
		if (mux_sel->pdata->mux_sel_2_type & CHG_THM_MUX_SEL)
			CHG_THM_MUX_SEL_NUM = SEC_MUX_SEL_2;
		if (mux_sel->pdata->mux_sel_2_type & SLAVE_CHG_THM_MUX_SEL)
			SLAVE_CHG_THM_MUX_SEL_NUM = SEC_MUX_SEL_2;
		if (mux_sel->pdata->mux_sel_2_type & AP_THM_MUX_SEL)
			AP_THM_MUX_SEL_NUM = SEC_MUX_SEL_2;
		if (mux_sel->pdata->mux_sel_2_type & WPC_THM_MUX_SEL)
			WPC_THM_MUX_SEL_NUM = SEC_MUX_SEL_2;
		if (mux_sel->pdata->mux_sel_2_type & BLKT_THM_MUX_SEL)
			BLKT_THM_MUX_SEL_NUM = SEC_MUX_SEL_2;	
	}

	if (mux_sel->pdata->mux_sel_3_en) {
		ret = gpio_request(mux_sel->pdata->mux_sel_2, "mux-sel3");
		if (ret) {
			pr_err("failed to request GPIO %u\n", mux_sel->pdata->mux_sel_3);
		}

		if (mux_sel->pdata->mux_sel_3_type & EAR_ADC_MUX_SEL)
			EAR_ADC_MUX_SEL_NUM = SEC_MUX_SEL_3;
		if (mux_sel->pdata->mux_sel_3_type & BATT_ID_MUX_SEL)
			BATT_ID_MUX_SEL_NUM = SEC_MUX_SEL_3;
		if (mux_sel->pdata->mux_sel_3_type & BATT_THM_MUX_SEL)
			BATT_THM_MUX_SEL_NUM = SEC_MUX_SEL_3;
		if (mux_sel->pdata->mux_sel_3_type & CHG_THM_MUX_SEL)
			CHG_THM_MUX_SEL_NUM = SEC_MUX_SEL_3;
		if (mux_sel->pdata->mux_sel_3_type & SLAVE_CHG_THM_MUX_SEL)
			SLAVE_CHG_THM_MUX_SEL_NUM = SEC_MUX_SEL_3;
		if (mux_sel->pdata->mux_sel_3_type & AP_THM_MUX_SEL)
			AP_THM_MUX_SEL_NUM = SEC_MUX_SEL_3;
		if (mux_sel->pdata->mux_sel_3_type & WPC_THM_MUX_SEL)
			WPC_THM_MUX_SEL_NUM = SEC_MUX_SEL_3;
		if (mux_sel->pdata->mux_sel_3_type & BLKT_THM_MUX_SEL)
			BLKT_THM_MUX_SEL_NUM = SEC_MUX_SEL_3;	
	}
	
	pr_info("%s EAR SEL NUM = %d, BATT ID SEL NUM = %d, BATT SEL NUM = %d, CHG SEL NUM = %d, "
		"AP SEL NUM = %d, WPC SEL NUM = %d, SLAVE CHG SEL NUM = %d, BLKT CHG SEL NUM = %d\n",
	__func__,
	EAR_ADC_MUX_SEL_NUM, 
	BATT_ID_MUX_SEL_NUM,
	BATT_THM_MUX_SEL_NUM,
	CHG_THM_MUX_SEL_NUM,
	AP_THM_MUX_SEL_NUM,
	WPC_THM_MUX_SEL_NUM,
	SLAVE_CHG_THM_MUX_SEL_NUM,
	BLKT_THM_MUX_SEL_NUM);
}

static int sec_mux_sel_probe(struct platform_device *pdev)
{
	sec_mux_sel_platform_data_t *pdata = NULL;
//	struct sec_mux_sel_info *mux_sel;
	int ret = 0;

	dev_info(&pdev->dev,
		"%s: SEC Mux Sel Driver Loading\n", __func__);

	mux_sel = kzalloc(sizeof(*mux_sel), GFP_KERNEL);
	if (!mux_sel)
		return -ENOMEM;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				sizeof(sec_mux_sel_platform_data_t),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_bat_free;
		}

		mux_sel->pdata = pdata;
		if (sec_mux_sel_parse_dt(&pdev->dev, mux_sel)) {
			dev_err(&pdev->dev,
				"%s: Failed to get mux sel dt\n", __func__);
			ret = -EINVAL;
			goto err_bat_free;
		}
	} else {
		pdata = dev_get_platdata(&pdev->dev);
		mux_sel->pdata = pdata;
	}

	platform_set_drvdata(pdev, mux_sel);
	mux_sel->dev = &pdev->dev;

	mutex_init(&mux_sel->mpp_share_mutex);
	sec_mux_sel_get_info();

	dev_info(mux_sel->dev,
		"%s: SEC Mux Sel Driver Loaded\n", __func__);

	return 0;

err_bat_free:
	kfree(mux_sel);

	return ret;
}

static int sec_mux_sel_remove(struct platform_device *pdev)
{
	struct sec_mux_sel_info *mux_sel = platform_get_drvdata(pdev);

	dev_dbg(mux_sel->dev, "%s: Start\n", __func__);

	mutex_destroy(&mux_sel->mpp_share_mutex);

	dev_dbg(mux_sel->dev, "%s: End\n", __func__);
	kfree(mux_sel);

	return 0;
}

static int sec_mux_sel_suspend(struct device *dev)
{
	return 0;
}

static int sec_mux_sel_resume(struct device *dev)
{
	return 0;
}

static void sec_mux_sel_shutdown(struct device *dev)
{
	return;
}

#ifdef CONFIG_OF
static struct of_device_id sec_mux_sel_dt_ids[] = {
	{ .compatible = "samsung,sec-mux-sel" },
	{ }
};
MODULE_DEVICE_TABLE(of, sec_mux_sel_dt_ids);
#endif /* CONFIG_OF */

static const struct dev_pm_ops sec_mux_sel_pm_ops = {
	.suspend = sec_mux_sel_suspend,
	.resume = sec_mux_sel_resume,
};

static struct platform_driver sec_mux_sel_driver = {
	.driver = {
		   .name = "sec-mux-sel",
		   .owner = THIS_MODULE,
		   .pm = &sec_mux_sel_pm_ops,
		   .shutdown = sec_mux_sel_shutdown,
#ifdef CONFIG_OF
		.of_match_table = sec_mux_sel_dt_ids,
#endif
		   },
	.probe = sec_mux_sel_probe,
	.remove = sec_mux_sel_remove,
};

static int __init sec_mux_sel_init(void)
{
	int ret;

	pr_info("%s \n", __func__);
	ret = platform_driver_register(&sec_mux_sel_driver);

	return ret;
}

static void __exit sec_mux_sel_exit(void)
{
	platform_driver_unregister(&sec_mux_sel_driver);
}

module_init(sec_mux_sel_init);
module_exit(sec_mux_sel_exit);

MODULE_DESCRIPTION("Samsung MUX SEL Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
