/*
 *sec_debug_test.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/cpu.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <soc/samsung/exynos-pmu.h>

typedef void (*force_error_func)(void);

static void simulate_KP(void);
static void simulate_DP(void);
static void simulate_WP(void);
static void simulate_TP(void);
static void simulate_PABRT(void);
static void simulate_UNDEF(void);
static void simulate_DFREE(void);
static void simulate_DREF(void);
static void simulate_MCRPT(void);
static void simulate_LOMEM(void);

enum {
	FORCE_KERNEL_PANIC = 0,      /* KP */
	FORCE_WATCHDOG,              /* DP */
	FORCE_WARM_RESET,            /* WP */
	FORCE_HW_TRIPPING,           /* TP */
	FORCE_PREFETCH_ABORT,        /* PABRT */
	FORCE_UNDEFINED_INSTRUCTION, /* UNDEF */
	FORCE_DOUBLE_FREE,           /* DFREE */
	FORCE_DANGLING_REFERENCE,    /* DREF */
	FORCE_MEMORY_CORRUPTION,     /* MCRPT */
	FORCE_LOW_MEMEMORY,          /* LOMEM */
	NR_FORCE_ERROR,
};

struct force_error_item {
	char errname[SZ_32];
	force_error_func errfunc;
};

struct force_error {
	struct force_error_item item[NR_FORCE_ERROR];
};

struct force_error force_error_vector = {
	.item = {
		{"KP",          &simulate_KP},
		{"DP",          &simulate_DP},
		{"WP",          &simulate_WP},
		{"TP",          &simulate_TP},
		{"pabrt",       &simulate_PABRT},
		{"undef",       &simulate_UNDEF},
		{"dfree",       &simulate_DFREE},
		{"danglingref", &simulate_DREF},
		{"memcorrupt",  &simulate_MCRPT},
		{"lowmem",      &simulate_LOMEM},
	}
};

/* timeout for dog bark/bite */
#define DELAY_TIME 30000
#define EXYNOS_PS_HOLD_CONTROL 0x330c

static void pull_down_other_cpus(void)
{
#ifdef CONFIG_HOTPLUG_CPU
	int cpu;

	for_each_online_cpu(cpu) {
		if (cpu == 0)
			continue;
		cpu_down(cpu);
	}
#endif
}

static void simulate_KP(void)
{
	pr_crit("%s()\n", __func__);
	*(unsigned int *)0x0 = 0x0; /* SVACE: intended */
}

static void simulate_DP(void)
{
	pr_crit("%s()\n", __func__);
	pull_down_other_cpus();
	local_irq_disable();
	mdelay(DELAY_TIME);
	local_irq_enable();

	/* should not reach here */
	pr_crit("%s() failed\n", __func__);
}

static void simulate_WP(void)
{
	unsigned int ps_hold_control;

	pr_crit("%s()\n", __func__);
	exynos_pmu_read(EXYNOS_PS_HOLD_CONTROL, &ps_hold_control);
	exynos_pmu_write(EXYNOS_PS_HOLD_CONTROL, ps_hold_control & 0xFFFFFEFF);
}

static void simulate_TP(void)
{
	pr_crit("%s()\n", __func__);
}

static void simulate_PABRT(void)
{
	pr_crit("%s()\n", __func__);
	((void (*)(void))0x0)(); /* SVACE: intended */
}

static void simulate_UNDEF(void)
{
	pr_crit("%s()\n", __func__);
	BUG();
}

static void simulate_DFREE(void)
{
	void *p;

	pr_crit("%s()\n", __func__);
	p = kmalloc(sizeof(unsigned int), GFP_KERNEL);
	if (p) {
		*(unsigned int *)p = 0x0;
		kfree(p);
		msleep(1000);
		kfree(p); /* SVACE: intended */
	}
}

static void simulate_DREF(void)
{
	unsigned int *p;

	pr_crit("%s()\n", __func__);
	p = kmalloc(sizeof(int), GFP_KERNEL);
	if (p) {
		kfree(p);
		*p = 0x1234; /* SVACE: intended */
	}
}

static void simulate_MCRPT(void)
{
	int *ptr;

	pr_crit("%s()\n", __func__);
	ptr = kmalloc(sizeof(int), GFP_KERNEL);
	if (ptr) {
		*ptr++ = 4;
		*ptr = 2;
		panic("MEMORY CORRUPTION");
	}
}

static void simulate_LOMEM(void)
{
	int i = 0;

	pr_crit("%s()\n", __func__);
	pr_crit("Allocating memory until failure!\n");
	while (kmalloc(128 * 1024, GFP_KERNEL)) /* SVACE: intended */
		i++;
	pr_crit("Allocated %d KB!\n", i * 128);
}

int sec_debug_force_error(const char *val, struct kernel_param *kp)
{
	int i;

	for (i = 0; i < NR_FORCE_ERROR; i++)
		if (!strncmp(val, force_error_vector.item[i].errname,
			     strlen(force_error_vector.item[i].errname)))
			force_error_vector.item[i].errfunc();
	return 0;
}
EXPORT_SYMBOL(sec_debug_force_error);
