/* sec_abc.c
 *
 * Abnormal Behavior Catcher Driver
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * Hyeokseon Yu <hyeokseon.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/sec_abc.h>

#define DEBUG_ABC

static struct device *sec_abc;
static char fail_result[ABC_BUFFER_MAX];
static int abc_enabled;
static int abc_probed;

#define ABC_PRINT(format, ...) pr_info("[sec_abc] " format, ##__VA_ARGS__)

#ifdef CONFIG_OF
static int parse_gpu_data(struct device *dev,
			  struct abc_platform_data *pdata,
			  struct device_node *np)
{
	struct abc_qdata *cgpu;

	cgpu = pdata->gpu_items;
	cgpu->desc = of_get_property(np, "gpu,label", NULL);

	if (of_property_read_u32(np, "gpu,threshold_count", &cgpu->threshold_cnt)) {
		dev_err(dev, "Failed to get gpu threshold count: node not exist\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "gpu,threshold_time", &cgpu->threshold_time)) {
		dev_err(dev, "Failed to get gpu threshold time: node not exist\n");
		return -EINVAL;
	}

	cgpu->buffer.abc_element = kzalloc(sizeof(cgpu->buffer.abc_element) * cgpu->threshold_cnt + 1, GFP_KERNEL);

	if (!cgpu->buffer.abc_element)
		return -ENOMEM;

	cgpu->buffer.size = cgpu->threshold_cnt + 1;
	cgpu->buffer.rear = 0;
	cgpu->buffer.front = 0;

	return 0;
}

static int parse_aicl_data(struct device *dev,
			  struct abc_platform_data *pdata,
			  struct device_node *np)
{
	struct abc_qdata *caicl;

	caicl = pdata->aicl_items;
	caicl->desc = of_get_property(np, "aicl,label", NULL);

	if (of_property_read_u32(np, "aicl,threshold_count", &caicl->threshold_cnt)) {
		dev_err(dev, "Failed to get aicl threshold count: node not exist\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "aicl,threshold_time", &caicl->threshold_time)) {
		dev_err(dev, "Failed to get aicl threshold time: node not exist\n");
		return -EINVAL;
	}

	caicl->buffer.abc_element = kzalloc(sizeof(caicl->buffer.abc_element) * caicl->threshold_cnt + 1, GFP_KERNEL);

	if (!caicl->buffer.abc_element)
		return -ENOMEM;

	caicl->buffer.size = caicl->threshold_cnt + 1;
	caicl->buffer.rear = 0;
	caicl->buffer.front = 0;

	return 0;
}

static int abc_parse_dt(struct device *dev)
{
	struct abc_platform_data *pdata = dev->platform_data;
	struct device_node *np;
	struct device_node *gpu_np;
	struct device_node *aicl_np;

	np = dev->of_node;
	pdata->nItem = of_get_child_count(np);
	if (!pdata->nItem) {
		dev_err(dev, "There are no items\n");
		return -ENODEV;
	}

	gpu_np = of_find_node_by_name(np, "gpu");
	pdata->nGpu = of_get_child_count(gpu_np);
	pdata->gpu_items = devm_kzalloc(dev,
					sizeof(struct abc_qdata) * pdata->nGpu, GFP_KERNEL);

	if (!pdata->gpu_items) {
		dev_err(dev, "Failed to allocate GPU memory\n");
		return -ENOMEM;
	}

	if (gpu_np)
		parse_gpu_data(dev, pdata, gpu_np);

	aicl_np = of_find_node_by_name(np, "aicl");
	pdata->nAicl = of_get_child_count(aicl_np);
	pdata->aicl_items = devm_kzalloc(dev,
					sizeof(struct abc_qdata) * pdata->nAicl, GFP_KERNEL);

	if (!pdata->aicl_items) {
		dev_err(dev, "Failed to allocate GPU memory\n");
		return -ENOMEM;
	}

	if (aicl_np)
		parse_aicl_data(dev, pdata, aicl_np);

	return 0;
}
#endif

#ifdef CONFIG_OF
static const struct of_device_id sec_abc_dt_match[] = {
	{ .compatible = "samsung,sec_abc" },
	{ }
};
#endif

static int sec_abc_resume(struct device *dev)
{
	return 0;
}

static int sec_abc_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct dev_pm_ops sec_abc_pm = {
	.resume = sec_abc_resume,
};

static ssize_t store_abc_enabled(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1)) {
		ABC_PRINT("ABC driver enabled.\n");
		abc_enabled = ABC_ENABLE_MAGIC;
	} else if (!strncmp(buf, "0", 1)) {
		ABC_PRINT("ABC driver disabled.\n");
		abc_enabled = 0;
	}
	return count;
}

static ssize_t show_abc_enabled(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%d\n", abc_enabled);
}
static DEVICE_ATTR(enabled, 0644, show_abc_enabled, store_abc_enabled);

static ssize_t show_abc_result(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	char return_str[ABC_BUFFER_MAX];

	/* Erase fail result when ACT app read file */
	strcpy(return_str, fail_result);
	memset(&fail_result, 0, sizeof(fail_result));
	sprintf(fail_result, "1");

	ABC_PRINT("Read data from ACT. result(%s)\n", return_str);

	return sprintf(buf, "%s\n", return_str);
}
static DEVICE_ATTR(result, 0444, show_abc_result, NULL);

static int sec_abc_is_full(struct abc_buffer *buffer)
{
	if ((buffer->rear + 1) % buffer->size == buffer->front)
		return 1;
	else
		return 0;
}

static int sec_abc_is_empty(struct abc_buffer *buffer)
{
	if (buffer->front == buffer->rear)
		return 1;
	else
		return 0;
}

static void sec_abc_enqueue(struct abc_buffer *buffer, struct abc_fault_info in)
{
	if (sec_abc_is_full(buffer)) {
		ABC_PRINT("queue is full.\n");
	} else {
		buffer->rear = (buffer->rear + 1) % buffer->size;
		buffer->abc_element[buffer->rear] = in;
	}
}

static void sec_abc_dequeue(struct abc_buffer *buffer, struct abc_fault_info *out)
{
	if (sec_abc_is_empty(buffer)) {
		ABC_PRINT("queue is empty.\n");
	} else {
		buffer->front = (buffer->front + 1) % buffer->size;
		*out = buffer->abc_element[buffer->front];
	}
}

static int sec_abc_get_diff_time(struct abc_buffer *buffer)
{
	int front_time, rear_time;

	front_time = buffer->abc_element[(buffer->front + 1) % buffer->size].cur_time;
	rear_time = buffer->abc_element[buffer->rear].cur_time;

	ABC_PRINT("front time : %d(%d) rear_time %d(%d) diff : %d\n",
		  front_time,
		  buffer->front + 1,
		  rear_time,
		  buffer->rear,
		  rear_time - front_time);

	return rear_time - front_time;
}

int sec_abc_get_magic(void)
{
	return abc_enabled;
}
EXPORT_SYMBOL(sec_abc_get_magic);

/* event string format
 *
 * ex) MODULE=tsp@ERROR=ghost_touch_holding@EXT_LOG=fw_ver(0108)
 *
 */
void sec_abc_send_event(char *str)
{
	struct abc_info *pinfo;
	struct abc_qdata *pgpu;
	struct abc_qdata *paicl;
	struct abc_fault_info in, out;

	static int gpu_fail_cnt;
	static int aicl_fail_cnt;
	char *token, *c, *p;
	char *uevent_str[ABC_UEVENT_MAX] = {0,};
	char temp[ABC_BUFFER_MAX], timestamp[ABC_BUFFER_MAX], event_type[ABC_BUFFER_MAX];
	int idx = 0, curr_time;
	ktime_t calltime;
	u64 realtime;

	if (!abc_probed) {
		ABC_PRINT("ABC driver is not probed!\n");
		return;
	}

	pinfo = dev_get_drvdata(sec_abc);
	pgpu = pinfo->pdata->gpu_items;
	paicl = pinfo->pdata->aicl_items;

	if (abc_enabled != ABC_ENABLE_MAGIC) {
		ABC_PRINT("Not support ABC driver!\n");
		return;
	}

	token = (char *)str;
	strcpy(temp, token);
	p = &temp[0];

	/* Caculate current time */
	calltime = ktime_get();
	realtime = ktime_to_ns(calltime);
	do_div(realtime, NSEC_PER_USEC);

	curr_time = realtime / USEC_PER_MSEC;

	/* Parse uevent string */
	while ((c = strsep(&p, "@")) != NULL) {
		uevent_str[idx] = c;
		idx++;
	}
	sprintf(timestamp, "TIMESTAMP=%d", curr_time);
	uevent_str[idx++] = &timestamp[0];
	uevent_str[idx] = '\0';
	strcpy(event_type, uevent_str[1] + 6);

	ABC_PRINT("event type : %s\n", event_type);

#if defined(DEBUG_ABC)
	idx = 0;
	while ((c = uevent_str[idx]) != '\0') {
		ABC_PRINT("%s\n", uevent_str[idx]);
		idx++;
	}
#endif

	/* GPU fault */
	if (!strncasecmp(event_type, "gpu_fault", 9)) {
		in.cur_time = realtime / USEC_PER_SEC;
		in.cur_cnt = gpu_fail_cnt++;

		ABC_PRINT("gpu fail count : %d\n", gpu_fail_cnt);
		sec_abc_enqueue(&pgpu->buffer, in);

		/* Check gpu fault */
		/* Case 1 : Over threshold count */
		if (gpu_fail_cnt >= pgpu->threshold_cnt) {
			if (sec_abc_get_diff_time(&pgpu->buffer) < pgpu->threshold_time) {
				ABC_PRINT("GPU fault occurred. Send uevent.\n");
				kobject_uevent_env(&sec_abc->kobj, KOBJ_CHANGE, uevent_str);
				sprintf(fail_result, "0_%s", token);
#if defined(DEBUG_ABC)
				ABC_PRINT("%s\n", fail_result);
#endif
			}
			gpu_fail_cnt = 0;
			sec_abc_dequeue(&pgpu->buffer, &out);
			ABC_PRINT("cur_time : %d cur_cnt : %d\n", out.cur_time, out.cur_cnt);
		} else if (sec_abc_is_full(&pgpu->buffer)) {
			/* Case 2 : Check front and rear node in queue. Because it's occurred within max count */
			if (sec_abc_get_diff_time(&pgpu->buffer) < pgpu->threshold_time) {
				ABC_PRINT("GPU fault occurred. Send uevent.\n");
				kobject_uevent_env(&sec_abc->kobj, KOBJ_CHANGE, uevent_str);
				sprintf(fail_result, "0_%s", token);
#if defined(DEBUG_ABC)
				ABC_PRINT("%s\n", fail_result);
#endif
			}
			sec_abc_dequeue(&pgpu->buffer, &out);
			ABC_PRINT("cur_time : %d cur_cnt : %d\n", out.cur_time, out.cur_cnt);
		}
	} else if (!strncasecmp(event_type, "aicl", 4)) { /* AICL fault */
		in.cur_time = realtime / USEC_PER_SEC;
		in.cur_cnt = aicl_fail_cnt++;

		ABC_PRINT("aicl fail count : %d\n", aicl_fail_cnt);
		sec_abc_enqueue(&paicl->buffer, in);

		/* Check aicl fault */
		if (aicl_fail_cnt >= paicl->threshold_cnt) {
			if (sec_abc_get_diff_time(&paicl->buffer) < paicl->threshold_time) {
				ABC_PRINT("AICL fault occurred. Send uevent.\n");
				kobject_uevent_env(&sec_abc->kobj, KOBJ_CHANGE, uevent_str);
				sprintf(fail_result, "0_%s", token);
#if defined(DEBUG_ABC)
				ABC_PRINT("%s\n", fail_result);
#endif
				while (!sec_abc_is_empty(&paicl->buffer))
					sec_abc_dequeue(&paicl->buffer, &out);
				aicl_fail_cnt = 0;
			} else {
				aicl_fail_cnt--;
				sec_abc_dequeue(&paicl->buffer, &out);
				ABC_PRINT("cur_time : %d cur_cnt : %d\n", out.cur_time, out.cur_cnt);
			}
		}
	} else {
		/* Others */
		kobject_uevent_env(&sec_abc->kobj, KOBJ_CHANGE, uevent_str);
		sprintf(fail_result, "0_%s", token);

		ABC_PRINT("Send uevent.\n");
	}
}
EXPORT_SYMBOL(sec_abc_send_event);

static int sec_abc_probe(struct platform_device *pdev)
{
	struct abc_platform_data *pdata;
	struct abc_info *pinfo;
	int ret = 0;

	ABC_PRINT("%s\n", __func__);

	abc_probed = false;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				     sizeof(struct abc_platform_data), GFP_KERNEL);

		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate platform data\n");
			return -ENOMEM;
		}

		pdev->dev.platform_data = pdata;
		ret = abc_parse_dt(&pdev->dev);
		if (ret) {
			dev_err(&pdev->dev, "Failed to parse dt data\n");
			return ret;
		}

		pr_info("%s: parse dt done\n", __func__);
	} else {
		pdata = pdev->dev.platform_data;
	}

	if (!pdata) {
		dev_err(&pdev->dev, "There are no platform data\n");
		return -EINVAL;
	}

	pinfo = devm_kzalloc(&pdev->dev, sizeof(struct abc_info), GFP_KERNEL);

	if (!pinfo)
		return -ENOMEM;

	pinfo->dev = sec_device_create(pinfo, "sec_abc");
	if (IS_ERR(pinfo->dev)) {
		pr_err("%s Failed to create device(sec_abc)!\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	ret = device_create_file(pinfo->dev, &dev_attr_enabled);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_abc_sysfs;
	}

	ret = device_create_file(pinfo->dev, &dev_attr_result);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_abc_sysfs;
	}

	/* Initialize fail result */
	sprintf(fail_result, "1");

	sec_abc = pinfo->dev;
	pinfo->pdata = pdata;

	platform_set_drvdata(pdev, pinfo);

	abc_probed = true;
	return ret;
err_create_abc_sysfs:
	sec_device_destroy(sec_abc->devt);
out:
	kfree(pinfo);
	kfree(pdata);

	return ret;
}

static struct platform_driver sec_abc_driver = {
	.probe = sec_abc_probe,
	.remove = sec_abc_remove,
	.driver = {
		.name = "sec_abc",
		.owner = THIS_MODULE,
#if defined(CONFIG_PM)
		.pm	= &sec_abc_pm,
#endif
#if CONFIG_OF
		.of_match_table = of_match_ptr(sec_abc_dt_match),
#endif
	},
};

static int __init sec_abc_init(void)
{
	ABC_PRINT("%s\n", __func__);

	return platform_driver_register(&sec_abc_driver);
}

static void __exit sec_abc_exit(void)
{
	return platform_driver_unregister(&sec_abc_driver);
}

module_init(sec_abc_init);
module_exit(sec_abc_exit);

MODULE_DESCRIPTION("Samsung ABC Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
