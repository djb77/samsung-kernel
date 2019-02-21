/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * BUS Monitor Debugging Driver for Samsung EXYNOS SoC
 * By Hosung Kim (hosung0.kim@samsung.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/exynos-busmon.h>

#define BUSMON_REG_FAULTEN		(0x08)
#define BUSMON_REG_ERRVLD		(0x0C)
#define BUSMON_REG_ERRCLR		(0x10)
#define BUSMON_REG_ERRLOG0		(0x14)
#define BUSMON_REG_ERRLOG1		(0x18)
#define BUSMON_REG_ERRLOG2		(0x1C)
#define BUSMON_REG_ERRLOG3		(0x20)
#define BUSMON_REG_ERRLOG4		(0x24)
#define BUSMON_REG_ERRLOG5		(0x28)
#define BUSMON_EINVAL			(99)

#define START				(0)
#define END				(1)
#define ARRAY_BITS			(2)
#define ARRAY_SUBRANGE_MAX		(1024)

#define NEED_TO_CHECK			(0xCAFE)

/* Error Code Description */
static char *busmon_errcode[] = {
	"0x0, SLV (Error Detect by the Slave)",
	"0x1, DEC (Decode error)",
	"0x2, UNS (Access type unsupported in target NIU)",
	"0x3, DISC(Disconnected Target or NOC domain)",
	"0x4, SEC (Security error)",
	"0x5, HIDE(Hidden security error)",
	"0x6, TMO (Timeout error)",
	"Invalid errorcode",
};

/* Opcode Description */
static char *busmon_opcode[] = {
	"0x0, RD  (INCR Read)",
	"0x1, RDW (WRAP Read)",
	"0x2, RDEX(Exclusive Read)",
	"0x3, RDLK(Locked Read)",
	"0x4, WR  (INCR Write)",
	"0x5, WRW (WRAP Write)",
	"0x6, WREX(Exclusive Write)",
	"0x7, WRLK(Locked Write)",
	"Invalid opcode",
};

#define BUSMON_INIT_DESC_STRING		"init-desc"
#define BUSMON_TARGET_DESC_STRING	"target-desc"
#define BUSMON_USERSIGNAL_DESC_STRING	"usersignal-desc"
#define BUSMON_UNSUPPORTED_STRING	"unsupported"

struct busmon_timeout {
	char *name;
	void __iomem *regs;
	u32 enabled;
	u32 enable_bit;
	u32 range_bits[ARRAY_BITS];
	struct list_head list;
};

struct busmon_platdata {
	/* RouteID Information Bits */
	u32 init_bits[ARRAY_BITS];
	u32 target_bits[ARRAY_BITS];
	u32 sub_bits[ARRAY_BITS];
	u32 seq_bits[ARRAY_BITS];

	/* Registers Bits */
	u32 faulten_bits[ARRAY_BITS];
	u32 errvld_bits[ARRAY_BITS];
	u32 errclr_bits[ARRAY_BITS];
	u32 errlog0_lock_bits[ARRAY_BITS];
	u32 errlog0_opc_bits[ARRAY_BITS];
	u32 errlog0_errcode_bits[ARRAY_BITS];
	u32 errlog0_len1_bits[ARRAY_BITS];
	u32 errlog0_format_bits[ARRAY_BITS];
	u32 errlog1_bits[ARRAY_BITS];
	u32 errlog2_bits[ARRAY_BITS];
	u32 errlog3_bits[ARRAY_BITS];
	u32 errlog4_bits[ARRAY_BITS];
	u32 errlog5_bits[ARRAY_BITS];
	u32 errlog5_axcache_bits[ARRAY_BITS];
	u32 errlog5_axdomain_bits[ARRAY_BITS];
	u32 errlog5_axuser_bits[ARRAY_BITS];
	u32 errlog5_axprot_bits[ARRAY_BITS];
	u32 errlog5_axqos_bits[ARRAY_BITS];
	u32 errlog5_axsnoop_bits[ARRAY_BITS];

	u32 init_num;
	u32 target_num;
	u32 sub_num;
	u32 sub_array;

	u32 init_flow;
	u32 target_flow;
	u32 subrange;
	u64 target_addr;

	u32 enabled;

	u32 sub_index[ARRAY_SUBRANGE_MAX];
	u32 sub_addr[ARRAY_SUBRANGE_MAX];

	struct busmon_notifier notifier_info;

	/* timeout block list */
	struct list_head timeout_list;
};

struct busmon_dev {
	struct device			*dev;
	struct busmon_platdata		*pdata;
	struct of_device_id		*match;
	int				irq;
	int				id;
	void __iomem			*regs;
	spinlock_t			ctrl_lock;
};

struct busmon_panic_block {
	struct notifier_block nb_panic_block;
	struct busmon_dev *pdev;
};

/* declare notifier_list */
static ATOMIC_NOTIFIER_HEAD(busmon_notifier_list);

static const struct of_device_id busmon_dt_match[] = {
	{ .compatible = "samsung,exynos-busmonitor",
	  .data = NULL, },
	{},
};
MODULE_DEVICE_TABLE(of, busmon_dt_match);

static char* busmon_get_string(struct device_node *np,
				 const char* desc_str,
				 unsigned int desc_num)
{
	const char *desc_ret;
	int ret;

	ret = of_property_read_string_index(np, desc_str, desc_num,
			(const char **)&desc_ret);
	if (ret)
		desc_ret = NULL;

	return (char *)desc_ret;
}

static unsigned int busmon_get_bits(u32 *bits, unsigned int val)
{
	unsigned int ret = 0, i;

	/* If bits[START] has BUSMON_EINVAL value, it must be exit */
	if (bits[END] != BUSMON_EINVAL) {
		/* Make masking value by checking from start-bit to end-bit */
		for (i = bits[START]; i <= bits[END]; i++)
			ret = (ret | (1 << i));
	}
	return ret & val;
}

static void busmon_logging_dump_raw(struct busmon_dev *busmon)
{
	struct busmon_platdata *pdata = busmon->pdata;
	unsigned int errlog0, errlog1, errlog2, errlog3, errlog4, errlog5, opcode, errcode;
	unsigned int axcache, axdomain, axuser, axprot, axqos, axsnoop;
	char *init_desc, *target_desc, *user_desc;

	errlog0 = __raw_readl(busmon->regs + BUSMON_REG_ERRLOG0);
	errlog1 = __raw_readl(busmon->regs + BUSMON_REG_ERRLOG1);
	errlog2 = __raw_readl(busmon->regs + BUSMON_REG_ERRLOG2);
	errlog3 = __raw_readl(busmon->regs + BUSMON_REG_ERRLOG3);
	errlog4 = __raw_readl(busmon->regs + BUSMON_REG_ERRLOG4);
	errlog5 = __raw_readl(busmon->regs + BUSMON_REG_ERRLOG5);

	init_desc = busmon_get_string(busmon->dev->of_node,
					BUSMON_INIT_DESC_STRING, pdata->init_flow);
	target_desc = busmon_get_string(busmon->dev->of_node,
					BUSMON_TARGET_DESC_STRING, pdata->target_flow);
	opcode = busmon_get_bits(pdata->errlog0_opc_bits, errlog0) >>
					pdata->errlog0_opc_bits[START];
	errcode = busmon_get_bits(pdata->errlog0_errcode_bits, errlog0) >>
					pdata->errlog0_errcode_bits[START];
	axcache = busmon_get_bits(pdata->errlog5_axcache_bits, errlog5) >>
					pdata->errlog5_axcache_bits[START];
	axdomain = busmon_get_bits(pdata->errlog5_axdomain_bits, errlog5) >>
					pdata->errlog5_axdomain_bits[START];
	axuser = busmon_get_bits(pdata->errlog5_axuser_bits, errlog5) >>
					pdata->errlog5_axuser_bits[START];
	user_desc = busmon_get_string(busmon->dev->of_node,
					BUSMON_USERSIGNAL_DESC_STRING,
					(pdata->init_flow << 4) | axuser);
	axprot = busmon_get_bits(pdata->errlog5_axprot_bits, errlog5) >>
					pdata->errlog5_axprot_bits[START];
	axqos = busmon_get_bits(pdata->errlog5_axqos_bits, errlog5) >>
					pdata->errlog5_axqos_bits[START];
	axsnoop = busmon_get_bits(pdata->errlog5_axsnoop_bits, errlog5) >>
					pdata->errlog5_axsnoop_bits[START];

	/* Check overflow */
	if (ARRAY_SIZE(busmon_opcode) <= opcode)
		opcode = ARRAY_SIZE(busmon_opcode) - 1;
	if (ARRAY_SIZE(busmon_errcode) <= errcode)
		errcode = ARRAY_SIZE(busmon_errcode) - 1;

	dev_err(busmon->dev, "Error detected by BUS Monitor\n"
		"=======================================================\n");
	dev_err(busmon->dev,
		"\nDebugging Information (1)\n"
		"\tPath       : %s -> %s\n"
		"\topcode     : %s\n"
		"\tErrorCode  : %s\n"
		"\tLength     : 0x%x (bytes)\n"
		"\tAddress    : 0x%llx\n"
		"\tFormat     : 0x%x\n"
		"\tinitflow   : 0x%x\n"
		"\ttargetflow : 0x%x\n"
		"\tsubrange   : 0x%x\n"
		"=======================================================\n",
		IS_ERR_OR_NULL(init_desc) ? BUSMON_UNSUPPORTED_STRING : init_desc,
		IS_ERR_OR_NULL(target_desc) ? BUSMON_UNSUPPORTED_STRING : target_desc,
		busmon_opcode[opcode], busmon_errcode[errcode],
		(busmon_get_bits(pdata->errlog0_len1_bits, errlog0) >>
				pdata->errlog0_len1_bits[START]) + 1,
		pdata->target_addr,
		busmon_get_bits(pdata->errlog0_format_bits, errlog0) >>
				pdata->errlog0_format_bits[START],
		pdata->init_flow, pdata->target_flow, pdata->subrange);

	dev_err(busmon->dev,
		"\nDebugging information (2)\n"
		"\tAXUSER     : 0x%x, Master IP: %s\n"
		"\tAXCACHE    : 0x%x\n"
		"\tAXDOMAIN   : 0x%x\n"
		"\tAXPROT     : 0x%x\n"
		"\tAXQOS      : 0x%x\n"
		"\tAXSNOOP    : 0x%x\n"
		"=======================================================\n",
		axuser, IS_ERR_OR_NULL(user_desc) ? BUSMON_UNSUPPORTED_STRING : user_desc,
		axcache, axdomain, axprot, axqos, axsnoop);

	dev_err(busmon->dev,
		"\nErrlog Raw Registers\n"
		"\tErrLog0  : 0x%x\n"
		"\tErrLog1  : 0x%x\n"
		"\tErrLog2  : 0x%x\n"
		"\tErrLog3  : 0x%x\n"
		"\tErrLog4  : 0x%x\n"
		"\tErrLog5  : 0x%x\n"
		"=======================================================\n",
		errlog0, errlog1, errlog2, errlog3, errlog4, errlog5);

	if (!pdata->target_addr)
		dev_err(busmon->dev, "Address is not valid, Needs to check\n");

	/* Fill the information for notifier call funcion */
	pdata->notifier_info.init_desc = init_desc;
	pdata->notifier_info.target_desc = target_desc;
	pdata->notifier_info.masterip_desc = user_desc;
	pdata->notifier_info.masterip_idx = axuser;
	pdata->notifier_info.target_addr = pdata->target_addr;
}

static void busmon_logging_parse_route(struct busmon_dev *busmon)
{
	struct busmon_platdata *pdata = busmon->pdata;

	unsigned int init_id, target_id, sub_id, val, bits;
	unsigned int errlog3 = 0, errlog4 = 0, i;

	val = __raw_readl(busmon->regs + BUSMON_REG_ERRLOG1);
	bits = busmon_get_bits(pdata->errlog1_bits, val);

	init_id = busmon_get_bits(pdata->init_bits, bits) >> pdata->init_bits[START];
	target_id = busmon_get_bits(pdata->target_bits, bits) >> pdata->target_bits[START];
	sub_id = busmon_get_bits(pdata->sub_bits, bits) >> pdata->sub_bits[START];

	pdata->init_flow = init_id;
	pdata->target_flow = target_id;
	pdata->subrange = sub_id;

	/* Calculate target address */
	errlog3 = __raw_readl(busmon->regs + BUSMON_REG_ERRLOG3);
	errlog4 = __raw_readl(busmon->regs + BUSMON_REG_ERRLOG4);

	errlog3 = busmon_get_bits(pdata->errlog3_bits, errlog3) >> pdata->errlog3_bits[START];
	errlog4 = busmon_get_bits(pdata->errlog4_bits, errlog4) >> pdata->errlog4_bits[START];

	val = (init_id * (pdata->target_num * pdata->sub_num)) +
	       (target_id * pdata->sub_num) + sub_id;

	for (i = 0; i < pdata->sub_array; i++) {
		if (pdata->sub_index[i] == val) {
			if (pdata->sub_addr[i] == NEED_TO_CHECK) {
				pdata->target_addr = 0;
			} else {
				pdata->target_addr = ((u64)errlog4 << 32);
				pdata->target_addr |= (errlog3 + pdata->sub_addr[i]);
			}
			break;
		}
	}
}

static void busmon_logging_dump(struct busmon_dev *busmon)
{
	busmon_logging_parse_route(busmon);
	busmon_logging_dump_raw(busmon);
}

static irqreturn_t busmon_logging_irq(int irq, void *data)
{
	struct busmon_dev *busmon = (struct busmon_dev *)data;
	struct busmon_platdata *pdata = busmon->pdata;
	unsigned int bits;
	unsigned int val;

	/* Check error has been logged */
	val = __raw_readl(busmon->regs + BUSMON_REG_ERRVLD);
	bits = busmon_get_bits(pdata->errvld_bits, val);

	if (bits) {
		char *init_desc;

		dev_info(busmon->dev, "BUS monitor information: %d interrupt occurs.\n", (irq - 32));
		busmon_logging_dump(busmon);

		/* error clear */
		bits = busmon_get_bits(pdata->errclr_bits, 1);
		__raw_writel(bits, busmon->regs + BUSMON_REG_ERRCLR);

		/* This code is for finding out the source */
		init_desc = busmon_get_string(busmon->dev->of_node,
				BUSMON_INIT_DESC_STRING, pdata->init_flow);

		/* call notifier_call_chain of busmon */
		atomic_notifier_call_chain(&busmon_notifier_list, 0, &pdata->notifier_info);

		if (init_desc && !strncmp(init_desc, "CPU", strlen("CPU")))
			dev_err(busmon->dev, "Error detected by BUS monitor.\n");
		else
			panic("Error detected by BUS monitor.");
	}

	return IRQ_HANDLED;
}

void busmon_notifier_chain_register(struct notifier_block *block)
{
	atomic_notifier_chain_register(&busmon_notifier_list, block);
}

static int busmon_logging_panic_handler(struct notifier_block *nb,
				   unsigned long l, void *buf)
{
	struct busmon_panic_block *busmon_panic = (struct busmon_panic_block *)nb;
	struct busmon_dev *busmon = busmon_panic->pdev;
	struct busmon_platdata *pdata = busmon->pdata;
	unsigned int bits;
	unsigned int val;

	if (!IS_ERR_OR_NULL(busmon)) {
		/* Check error has been logged */
		val = __raw_readl(busmon->regs + BUSMON_REG_ERRVLD);
		bits = busmon_get_bits(pdata->errvld_bits, val);

		if (bits)
			busmon_logging_dump(busmon);
		else
			dev_info(busmon->dev,
				"BUS monitor did not detect any error.\n");
	}
	return 0;
}

static void busmon_timeout_init(struct busmon_dev *busmon)
{
	struct busmon_timeout *timeout;
	struct list_head *entry;
	u32 val;

	if (list_empty(&busmon->pdata->timeout_list))
		return;

	list_for_each(entry, &busmon->pdata->timeout_list) {
		timeout = list_entry(entry, struct busmon_timeout, list);
		if (timeout && timeout->enabled) {
			val = __raw_readl(timeout->regs);
			val |= (0x1) << timeout->enable_bit;
			__raw_writel(val, timeout->regs);

			dev_dbg(busmon->dev,
				"Exynos Bus Monitor timeout enabled(%s, bit:%d)\n",
				timeout->name, timeout->enable_bit);
		}
	}
}

static void busmon_logging_init(struct busmon_dev *busmon)
{
	struct busmon_platdata *pdata = busmon->pdata;
	unsigned int bits;

	if (pdata->enabled) {
		/* first of all, error clear at occurs previous */
		bits = busmon_get_bits(pdata->errclr_bits, 1);
		__raw_writel(bits, busmon->regs + BUSMON_REG_ERRCLR);

		/* enable logging init */
		bits = busmon_get_bits(pdata->faulten_bits, 1);
		__raw_writel(bits, busmon->regs + BUSMON_REG_FAULTEN);
	}
	dev_dbg(busmon->dev, "Exynos BUS Monitor logging %s\n",
			pdata->enabled ? "enabled" : "disabled");
}

static int busmon_dt_parse(struct device_node *np,
				struct busmon_dev *busmon)
{
	struct busmon_platdata *pdata = busmon->pdata;
	struct device_node *time_np, *time_child_np = NULL;
	struct busmon_timeout *timeout;
	u32 regs[2];
	int ret;

	if (!np || !pdata) {
		ret = -EINVAL;
		goto out;
	}

	/* Error logging enabled */
	of_property_read_u32(np, "enabled", &pdata->enabled);

	/* Read BUS Logging setting */
	of_property_read_u32_array(np, "seq-bits", pdata->seq_bits, 2);
	of_property_read_u32_array(np, "sub-bits", pdata->sub_bits, 2);
	of_property_read_u32_array(np, "target-bits", pdata->target_bits, 2);
	of_property_read_u32_array(np, "init-bits", pdata->init_bits, 2);

	of_property_read_u32_array(np, "faulten-bits", pdata->faulten_bits, 2);
	of_property_read_u32_array(np, "errvld-bits", pdata->errvld_bits, 2);
	of_property_read_u32_array(np, "errclr-bits", pdata->errclr_bits, 2);

	of_property_read_u32_array(np, "errlog0-lock-bits", pdata->errlog0_lock_bits, 2);
	of_property_read_u32_array(np, "errlog0-opc-bits", pdata->errlog0_opc_bits, 2);
	of_property_read_u32_array(np, "errlog0-errcode-bits", pdata->errlog0_errcode_bits, 2);
	of_property_read_u32_array(np, "errlog0-len1-bits", pdata->errlog0_len1_bits, 2);
	of_property_read_u32_array(np, "errlog0-format-bits", pdata->errlog0_format_bits, 2);
	of_property_read_u32_array(np, "errlog1-bits", pdata->errlog1_bits, 2);
	of_property_read_u32_array(np, "errlog2-bits", pdata->errlog2_bits, 2);
	of_property_read_u32_array(np, "errlog3-bits", pdata->errlog3_bits, 2);
	of_property_read_u32_array(np, "errlog4-bits", pdata->errlog4_bits, 2);
	of_property_read_u32_array(np, "errlog5-bits", pdata->errlog5_bits, 2);

	/* errlog5's slot bits are different for each */
	ret = of_property_read_u32_array(np, "errlog5-axcache-bits",
					pdata->errlog5_axcache_bits, 2);
	if (ret) {
		pdata->errlog5_axcache_bits[START] = 0;
		pdata->errlog5_axcache_bits[END] = BUSMON_EINVAL;
	}
	ret = of_property_read_u32_array(np, "errlog5-axdomain-bits",
					pdata->errlog5_axdomain_bits, 2);
	if (ret) {
		pdata->errlog5_axdomain_bits[START] = 0;
		pdata->errlog5_axdomain_bits[END] = BUSMON_EINVAL;
	}
	ret = of_property_read_u32_array(np, "errlog5-axuser-bits",
					pdata->errlog5_axuser_bits, 2);
	if (ret) {
		pdata->errlog5_axuser_bits[START] = 0;
		pdata->errlog5_axuser_bits[END] = BUSMON_EINVAL;
	}
	ret = of_property_read_u32_array(np, "errlog5-axprot-bits",
					pdata->errlog5_axprot_bits, 2);
	if (ret) {
		pdata->errlog5_axprot_bits[START] = 0;
		pdata->errlog5_axprot_bits[END] = BUSMON_EINVAL;
	}
	ret = of_property_read_u32_array(np, "errlog5-axqos-bits",
					pdata->errlog5_axqos_bits, 2);
	if (ret) {
		pdata->errlog5_axqos_bits[START] = 0;
		pdata->errlog5_axqos_bits[END] = BUSMON_EINVAL;
	}
	ret = of_property_read_u32_array(np, "errlog5-axsnoop-bits",
					pdata->errlog5_axsnoop_bits, 2);
	if (ret) {
		pdata->errlog5_axsnoop_bits[START] = 0;
		pdata->errlog5_axsnoop_bits[END] = BUSMON_EINVAL;
	}

	of_property_read_u32(np, "init-num", &pdata->init_num);
	of_property_read_u32(np, "target-num", &pdata->target_num);
	of_property_read_u32(np, "sub-num", &pdata->sub_num);
	of_property_read_u32(np, "sub-array", &pdata->sub_array);

	of_property_read_u32_array(np, "sub-index", pdata->sub_index, pdata->sub_array);
	of_property_read_u32_array(np, "sub-addr", pdata->sub_addr, pdata->sub_array);

	/* Mandatory parsing is done */
	ret = 0;

	/* Check BUS Timeout setting(Option) */
	INIT_LIST_HEAD(&pdata->timeout_list);
	time_np = of_get_child_by_name(np, "timeout");
	if (!time_np)
		goto out;

	/* BUS timeout setting */
	while ((time_child_np = of_get_next_child(time_np, time_child_np)) != NULL) {
		timeout = devm_kzalloc(busmon->dev,
					sizeof(struct busmon_timeout), GFP_KERNEL);
		if (!timeout) {
			dev_err(busmon->dev,
				"failed to allocate memory for busmon-timeout\n");
			continue;
		}
		if (of_property_read_string(time_child_np, "nickname",
					(const char **)&timeout->name)) {
			dev_err(busmon->dev,
					"failed to get nickname property\n");
			continue;
		}

		of_property_read_u32_array(time_child_np, "reg", regs, 2);
		timeout->regs = ioremap(regs[0], regs[1]);
		if (!timeout->regs) {
			dev_err(busmon->dev,
				"failed to ioremap for busmon-timeout: %s\n",
				timeout->name);
			devm_kfree(busmon->dev, timeout);
			continue;
		}
		of_property_read_u32(time_child_np, "enabled",
					&timeout->enabled);
		of_property_read_u32(time_child_np, "enable-bit",
					&timeout->enable_bit);
		of_property_read_u32_array(time_child_np, "range-bits",
					timeout->range_bits, 2);
		list_add(&timeout->list, &pdata->timeout_list);
	}
	of_node_put(time_np);
out:
	return ret;
}

static int busmon_probe(struct platform_device *pdev)
{
	struct busmon_dev *busmon;
	struct busmon_platdata *pdata = NULL;
	struct busmon_panic_block *busmon_panic = NULL;
	const struct of_device_id *match;
	struct resource *res;
	int ret;

	busmon = devm_kzalloc(&pdev->dev, sizeof(struct busmon_dev), GFP_KERNEL);
	if (!busmon) {
		dev_err(&pdev->dev, "failed to allocate memory for driver's "
				"private data\n");
		return -ENOMEM;
	}
	busmon->dev = &pdev->dev;
	match = of_match_node(busmon_dt_match, pdev->dev.of_node);
	busmon->match = (struct of_device_id *)match;

	spin_lock_init(&busmon->ctrl_lock);

	pdata = devm_kzalloc(&pdev->dev, sizeof(struct busmon_platdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(&pdev->dev, "failed to allocate memory for driver's "
				"platform data\n");
		return -ENOMEM;
	}
	busmon->pdata = pdata;

	ret = busmon_dt_parse(pdev->dev.of_node, busmon);
	if (ret) {
		dev_err(&pdev->dev, "failed to assign device tree parsing\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;

	busmon->regs = devm_ioremap_resource(&pdev->dev, res);
	if (busmon->regs == NULL) {
		dev_err(&pdev->dev, "failed to claim register region\n");
		return -ENOENT;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res)
		return -ENXIO;

	busmon->irq = res->start;
	ret = devm_request_irq(&pdev->dev, busmon->irq, busmon_logging_irq,
					0, dev_name(&pdev->dev), busmon);
	if (ret) {
		dev_err(&pdev->dev, "irq request failed\n");
		return -ENXIO;
	}

	busmon_panic = devm_kzalloc(&pdev->dev,
			sizeof(struct busmon_panic_block), GFP_KERNEL);
	if (!busmon_panic) {
		dev_err(&pdev->dev, "failed to allocate memory for driver's "
				"panic handler data\n");
	} else {
		busmon_panic->nb_panic_block.notifier_call =
					busmon_logging_panic_handler;
		busmon_panic->pdev = busmon;
		atomic_notifier_chain_register(&panic_notifier_list,
					&busmon_panic->nb_panic_block);
	}

	platform_set_drvdata(pdev, busmon);

	busmon_timeout_init(busmon);
	busmon_logging_init(busmon);

	return 0;
}

static int busmon_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int busmon_suspend(struct device *dev)
{
	return 0;
}

static int busmon_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct busmon_dev *busmon = platform_get_drvdata(pdev);

	busmon_timeout_init(busmon);
	busmon_logging_init(busmon);

	return 0;
}

static SIMPLE_DEV_PM_OPS(busmon_pm_ops,
			 busmon_suspend,
			 busmon_resume);
#define BUSMON_PM	(busmon_pm_ops)
#else
#define BUSMON_PM	NULL
#endif

static struct platform_driver exynos_busmon_driver = {
	.probe		= busmon_probe,
	.remove		= busmon_remove,
	.driver		= {
		.name		= "exynos-busmon",
		.of_match_table	= busmon_dt_match,
		.pm		= &busmon_pm_ops,
	},
};

module_platform_driver(exynos_busmon_driver);

MODULE_DESCRIPTION("Samsung Exynos BUS MONITOR DRIVER");
MODULE_AUTHOR("Hosung Kim <hosung0.kim@samsung.com");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:exynos-busmon");
