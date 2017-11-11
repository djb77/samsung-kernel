/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/time.h>
#include <linux/slab.h>

#include "vpu-config.h"
#include "vpu-hardware.h"
#include "lib/vpul-hwmapper.h"
#include "../vpu-device.h"
#include "../vpu-vertex.h"

enum {
	PUID_none = VPU_PU_TYPE_NONE,
	PUID_dmain = VPU_PU_TYPE_DMAIN,
	PUID_dmaot = VPU_PU_TYPE_DMAOT,
	PUID_salb = VPU_PU_TYPE_SALB,
	PUID_calb = VPU_PU_TYPE_CALB,
	PUID_rois = VPU_PU_TYPE_ROIS,
	PUID_crop = VPU_PU_TYPE_CROP,
	PUID_mde = VPU_PU_TYPE_MDE,
	PUID_map2list = VPU_PU_TYPE_MAP2LIST,
	PUID_nms = VPU_PU_TYPE_NMS,
	PUID_slf5 = VPU_PU_TYPE_SLF5,
	PUID_slf7 = VPU_PU_TYPE_SLF7,
	PUID_glf5 = VPU_PU_TYPE_GLF5,
	PUID_ccm = VPU_PU_TYPE_CCM,
	PUID_lut = VPU_PU_TYPE_LUT,
	PUID_integral = VPU_PU_TYPE_INTEGRAL,
	PUID_upscaler = VPU_PU_TYPE_UPSCALER,
	PUID_dnscaler = VPU_PU_TYPE_DNSCALER,
	PUID_joiner = VPU_PU_TYPE_JOINER,
	PUID_spliter = VPU_PU_TYPE_SPLITTER,
	PUID_duplicator = VPU_PU_TYPE_DUPLICATOR,
	PUID_histogram = VPU_PU_TYPE_HISTOGRAM,
	PUID_nlf = VPU_PU_TYPE_NLF,
	PUID_fastdepth = VPU_PU_TYPE_FASTDEPTH,
	PUID_disparity = VPU_PU_TYPE_DISPARITY,
	PUID_inpaint = VPU_PU_TYPE_INPAINT,
	PUID_cnn = VPU_PU_TYPE_CNN,
	PUID_fifo = VPU_PU_TYPE_FIFO
};

#define PU_BLOCK_DEFINE(_block, _name, _count) do { \
		_block.total = _count; \
		snprintf(_block.name, VPU_HW_MAX_NM_COUNT, "pu-%s", #_name); \
		_block.attr_group.name = _block.name; \
		_block.attr_group.attrs = _name##_attrs; \
	} while(0);

#define MPRB_BLOCK_DEFINE(_block, _name, _count) do { \
		_block.total = _count; \
		snprintf(_block.name, VPU_HW_MAX_NM_COUNT, "mprb-%s", #_name); \
		_block.attr_group.name = _block.name; \
		_block.attr_group.attrs = _name##_attrs; \
	} while(0);

#define __VPU_ATTR_RO(_name, _func) {					\
	.attr   = { .name = __stringify(_func), .mode = S_IRUGO },	\
	.show   = _name##_##_func##_show,				\
}

#define DECLARE_PU_SHOW(_name, _func) \
	static ssize_t _name##_##_func##_show(struct device *dev, struct device_attribute *attr, char *buf) \
	{ return __common_##_func##_show(dev, PUID_##_name, attr, buf); } \
	static struct device_attribute dev_attr_##_name##_##_func = __VPU_ATTR_RO(_name, _func)

#define DECLARE_MPRB_SHOW(_name, _func) \
	static ssize_t _name##_##_func##_show(struct device *dev, struct device_attribute *attr, char *buf) \
	{ return __common_##_func##_show(dev, MPRB_##_name, attr, buf); } \
	static struct device_attribute dev_attr_##_name##_##_func = __VPU_ATTR_RO(_name, _func)

#define DECLARE_ATTRIBUTE(_name) static struct attribute *_name##_attrs[] = { \
		&dev_attr_##_name##_map.attr, \
		&dev_attr_##_name##_count.attr, \
		&dev_attr_##_name##_avail.attr, \
		&dev_attr_##_name##_refer.attr, \
		NULL \
	}

static ssize_t __common_map_show(struct device *dev, u32 id,
	struct device_attribute *attr, char *buf)
{
	struct vpu_vertex *vertex = dev_get_drvdata(dev);
	struct vpu_device *device = container_of(vertex, struct vpu_device, vertex);
	struct vpu_hw_block *pu_block = &device->resource.hardware.pu.table[id];
	char bitmap[20];

	bitmap_scnprintf(bitmap, sizeof(bitmap), pu_block->allocated, pu_block->total);
	return snprintf(buf, sizeof(bitmap), "%s\n", bitmap);
}

static ssize_t __common_count_show(struct device *dev, u32 id,
	struct device_attribute *attr, char *buf)
{
	struct vpu_vertex *vertex = dev_get_drvdata(dev);
	struct vpu_device *device = container_of(vertex, struct vpu_device, vertex);
	struct vpu_hw_block *pu_block = &device->resource.hardware.pu.table[id];

	return snprintf(buf, sizeof(pu_block->total), "%d\n", pu_block->total);
}

static ssize_t __common_avail_show(struct device *dev, u32 id,
	struct device_attribute *attr, char *buf)
{
	struct vpu_vertex *vertex = dev_get_drvdata(dev);
	struct vpu_device *device = container_of(vertex, struct vpu_device, vertex);
	struct vpu_hw_block *pu_block = &device->resource.hardware.pu.table[id];
	u32 i, avail = 0;

	for (i = 0; i < pu_block->total; ++i) {
		if (!test_bit(i, pu_block->allocated))
			avail++;
	}

	return snprintf(buf, sizeof(avail), "%d\n", avail);
}

static ssize_t __common_refer_show(struct device *dev, u32 id,
	struct device_attribute *attr, char *buf)
{
	struct vpu_vertex *vertex = dev_get_drvdata(dev);
	struct vpu_device *device = container_of(vertex, struct vpu_device, vertex);
	struct vpu_hw_block *pu_block = &device->resource.hardware.pu.table[id];

	return snprintf(buf, sizeof(pu_block->refer), "%d\n", pu_block->refer);
}

DECLARE_PU_SHOW(dmain, map);
DECLARE_PU_SHOW(dmain, count);
DECLARE_PU_SHOW(dmain, avail);
DECLARE_PU_SHOW(dmain, refer);

DECLARE_PU_SHOW(dmaot, map);
DECLARE_PU_SHOW(dmaot, count);
DECLARE_PU_SHOW(dmaot, avail);
DECLARE_PU_SHOW(dmaot, refer);

DECLARE_PU_SHOW(salb, map);
DECLARE_PU_SHOW(salb, count);
DECLARE_PU_SHOW(salb, avail);
DECLARE_PU_SHOW(salb, refer);

DECLARE_PU_SHOW(calb, map);
DECLARE_PU_SHOW(calb, count);
DECLARE_PU_SHOW(calb, avail);
DECLARE_PU_SHOW(calb, refer);

DECLARE_PU_SHOW(rois, map);
DECLARE_PU_SHOW(rois, count);
DECLARE_PU_SHOW(rois, avail);
DECLARE_PU_SHOW(rois, refer);

DECLARE_PU_SHOW(crop, map);
DECLARE_PU_SHOW(crop, count);
DECLARE_PU_SHOW(crop, avail);
DECLARE_PU_SHOW(crop, refer);

DECLARE_PU_SHOW(mde, map);
DECLARE_PU_SHOW(mde, count);
DECLARE_PU_SHOW(mde, avail);
DECLARE_PU_SHOW(mde, refer);

DECLARE_PU_SHOW(map2list, map);
DECLARE_PU_SHOW(map2list, count);
DECLARE_PU_SHOW(map2list, avail);
DECLARE_PU_SHOW(map2list, refer);

DECLARE_PU_SHOW(nms, map);
DECLARE_PU_SHOW(nms, count);
DECLARE_PU_SHOW(nms, avail);
DECLARE_PU_SHOW(nms, refer);

DECLARE_PU_SHOW(slf5, map);
DECLARE_PU_SHOW(slf5, count);
DECLARE_PU_SHOW(slf5, avail);
DECLARE_PU_SHOW(slf5, refer);

DECLARE_PU_SHOW(slf7, map);
DECLARE_PU_SHOW(slf7, count);
DECLARE_PU_SHOW(slf7, avail);
DECLARE_PU_SHOW(slf7, refer);

DECLARE_PU_SHOW(glf5, map);
DECLARE_PU_SHOW(glf5, count);
DECLARE_PU_SHOW(glf5, avail);
DECLARE_PU_SHOW(glf5, refer);

DECLARE_PU_SHOW(ccm, map);
DECLARE_PU_SHOW(ccm, count);
DECLARE_PU_SHOW(ccm, avail);
DECLARE_PU_SHOW(ccm, refer);

DECLARE_PU_SHOW(lut, map);
DECLARE_PU_SHOW(lut, count);
DECLARE_PU_SHOW(lut, avail);
DECLARE_PU_SHOW(lut, refer);

DECLARE_PU_SHOW(integral, map);
DECLARE_PU_SHOW(integral, count);
DECLARE_PU_SHOW(integral, avail);
DECLARE_PU_SHOW(integral, refer);

DECLARE_PU_SHOW(upscaler, map);
DECLARE_PU_SHOW(upscaler, count);
DECLARE_PU_SHOW(upscaler, avail);
DECLARE_PU_SHOW(upscaler, refer);

DECLARE_PU_SHOW(dnscaler, map);
DECLARE_PU_SHOW(dnscaler, count);
DECLARE_PU_SHOW(dnscaler, avail);
DECLARE_PU_SHOW(dnscaler, refer);

DECLARE_PU_SHOW(joiner, map);
DECLARE_PU_SHOW(joiner, count);
DECLARE_PU_SHOW(joiner, avail);
DECLARE_PU_SHOW(joiner, refer);

DECLARE_PU_SHOW(spliter, map);
DECLARE_PU_SHOW(spliter, count);
DECLARE_PU_SHOW(spliter, avail);
DECLARE_PU_SHOW(spliter, refer);

DECLARE_PU_SHOW(duplicator, map);
DECLARE_PU_SHOW(duplicator, count);
DECLARE_PU_SHOW(duplicator, avail);
DECLARE_PU_SHOW(duplicator, refer);

DECLARE_PU_SHOW(histogram, map);
DECLARE_PU_SHOW(histogram, count);
DECLARE_PU_SHOW(histogram, avail);
DECLARE_PU_SHOW(histogram, refer);

DECLARE_PU_SHOW(nlf, map);
DECLARE_PU_SHOW(nlf, count);
DECLARE_PU_SHOW(nlf, avail);
DECLARE_PU_SHOW(nlf, refer);

DECLARE_PU_SHOW(fastdepth, map);
DECLARE_PU_SHOW(fastdepth, count);
DECLARE_PU_SHOW(fastdepth, avail);
DECLARE_PU_SHOW(fastdepth, refer);

DECLARE_PU_SHOW(disparity, map);
DECLARE_PU_SHOW(disparity, count);
DECLARE_PU_SHOW(disparity, avail);
DECLARE_PU_SHOW(disparity, refer);

DECLARE_PU_SHOW(inpaint, map);
DECLARE_PU_SHOW(inpaint, count);
DECLARE_PU_SHOW(inpaint, avail);
DECLARE_PU_SHOW(inpaint, refer);

DECLARE_PU_SHOW(cnn, map);
DECLARE_PU_SHOW(cnn, count);
DECLARE_PU_SHOW(cnn, avail);
DECLARE_PU_SHOW(cnn, refer);

DECLARE_PU_SHOW(fifo, map);
DECLARE_PU_SHOW(fifo, count);
DECLARE_PU_SHOW(fifo, avail);
DECLARE_PU_SHOW(fifo, refer);

DECLARE_MPRB_SHOW(large, map);
DECLARE_MPRB_SHOW(large, count);
DECLARE_MPRB_SHOW(large, avail);
DECLARE_MPRB_SHOW(large, refer);

DECLARE_MPRB_SHOW(small, map);
DECLARE_MPRB_SHOW(small, count);
DECLARE_MPRB_SHOW(small, avail);
DECLARE_MPRB_SHOW(small, refer);

DECLARE_ATTRIBUTE(dmain);
DECLARE_ATTRIBUTE(dmaot);
DECLARE_ATTRIBUTE(salb);
DECLARE_ATTRIBUTE(calb);
DECLARE_ATTRIBUTE(rois);
DECLARE_ATTRIBUTE(crop);
DECLARE_ATTRIBUTE(mde);
DECLARE_ATTRIBUTE(map2list);
DECLARE_ATTRIBUTE(nms);
DECLARE_ATTRIBUTE(slf5);
DECLARE_ATTRIBUTE(slf7);
DECLARE_ATTRIBUTE(glf5);
DECLARE_ATTRIBUTE(ccm);
DECLARE_ATTRIBUTE(lut);
DECLARE_ATTRIBUTE(integral);
DECLARE_ATTRIBUTE(upscaler);
DECLARE_ATTRIBUTE(dnscaler);
DECLARE_ATTRIBUTE(joiner);
DECLARE_ATTRIBUTE(spliter);
DECLARE_ATTRIBUTE(duplicator);
DECLARE_ATTRIBUTE(histogram);
DECLARE_ATTRIBUTE(nlf);
DECLARE_ATTRIBUTE(fastdepth);
DECLARE_ATTRIBUTE(disparity);
DECLARE_ATTRIBUTE(inpaint);
DECLARE_ATTRIBUTE(cnn);
DECLARE_ATTRIBUTE(fifo);
DECLARE_ATTRIBUTE(large);
DECLARE_ATTRIBUTE(small);

int vpu_hardware_host_init(struct vpu_hardware *hardware)
{
	int i;
	struct vpu_hw_block *blocks;

	blocks = hardware->pu.table;

	for (i = 0; i < hardware->pu.total; ++i) {
		blocks[i].attr_group.name = NULL;
		blocks[i].attr_group.attrs = NULL;
	}

	PU_BLOCK_DEFINE(blocks[PUID_dmain], dmain, 5);
	PU_BLOCK_DEFINE(blocks[PUID_dmaot], dmaot, 5);
	PU_BLOCK_DEFINE(blocks[PUID_salb], salb, 9);
	PU_BLOCK_DEFINE(blocks[PUID_calb], calb, 3);
	PU_BLOCK_DEFINE(blocks[PUID_rois], rois, 2);
	PU_BLOCK_DEFINE(blocks[PUID_crop], crop, 3);
	PU_BLOCK_DEFINE(blocks[PUID_mde], mde, 1);
	PU_BLOCK_DEFINE(blocks[PUID_map2list], map2list, 1);
	PU_BLOCK_DEFINE(blocks[PUID_nms], nms, 1);
	PU_BLOCK_DEFINE(blocks[PUID_slf5], slf5, 3);
	PU_BLOCK_DEFINE(blocks[PUID_slf7], slf7, 3);
	PU_BLOCK_DEFINE(blocks[PUID_glf5], glf5, 2);
	PU_BLOCK_DEFINE(blocks[PUID_ccm], ccm, 1);
	PU_BLOCK_DEFINE(blocks[PUID_lut], lut, 1);
	PU_BLOCK_DEFINE(blocks[PUID_integral], integral, 1);
	PU_BLOCK_DEFINE(blocks[PUID_upscaler], upscaler, 1);
	PU_BLOCK_DEFINE(blocks[PUID_dnscaler], dnscaler, 2);
	PU_BLOCK_DEFINE(blocks[PUID_joiner], joiner, 2);
	PU_BLOCK_DEFINE(blocks[PUID_spliter], spliter, 2);
	PU_BLOCK_DEFINE(blocks[PUID_duplicator], duplicator, 5);
	PU_BLOCK_DEFINE(blocks[PUID_histogram], histogram, 1);
	PU_BLOCK_DEFINE(blocks[PUID_nlf], nlf, 1);
	PU_BLOCK_DEFINE(blocks[PUID_fastdepth], fastdepth, 1);
	PU_BLOCK_DEFINE(blocks[PUID_disparity], disparity, 1);
	PU_BLOCK_DEFINE(blocks[PUID_inpaint], inpaint, 1);
	PU_BLOCK_DEFINE(blocks[PUID_cnn], cnn, 1);
	PU_BLOCK_DEFINE(blocks[PUID_fifo], fifo, 13);

	blocks = hardware->mprb.table;

	for (i = 0; i < hardware->mprb.total; ++i) {
		blocks[i].attr_group.name = NULL;
		blocks[i].attr_group.attrs = NULL;
	}

	MPRB_BLOCK_DEFINE(blocks[MPRB_large], large, 24);
	MPRB_BLOCK_DEFINE(blocks[MPRB_small], small, 23);

	return 0;
}