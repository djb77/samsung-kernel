/*
 * Copyright (c) 2014-2015 TRUSTONIC LIMITED
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/types.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <video/s3c-fb.h>
#include <t-base-tui.h>

#include "tui_ioctl.h"
#include "dciTui.h"
#include "tlcTui.h"
#include "tui-hal.h"


#define TUI_MEMPOOL_SIZE 0

struct tui_mempool {
	void *va;
	unsigned long pa;
	size_t size;
};

static struct tui_mempool g_tui_mem_pool;

static bool allocate_tui_memory_pool(struct tui_mempool *pool, size_t size)
{
	bool ret = false;
	void *tui_mem_pool = NULL;

	pr_info("%s %s:%d\n", __func__, __FILE__, __LINE__);
	if (!size) {
		pr_debug("TUI frame buffer: nothing to allocate.");
		return true;
	}

	tui_mem_pool = kmalloc(size, GFP_KERNEL);
	if (!tui_mem_pool) {
		pr_debug("ERROR Could not allocate TUI memory pool");
	} else if (ksize(tui_mem_pool) < size) {
		pr_err("TUI mem pool size too small: req'd=%d alloc'd=%d", size,
		       ksize(tui_mem_pool));
		kfree(tui_mem_pool);
	} else {
		pool->va = tui_mem_pool;
		pool->pa = virt_to_phys(tui_mem_pool);
		pool->size = ksize(tui_mem_pool);
		ret = true;
	}
	return ret;
}

static void free_tui_memory_pool(struct tui_mempool *pool)
{
	kfree(pool->va);
	memset(pool, 0, sizeof(*pool));
}

static int is_device_ok(struct device *fbdev, void *p)
{
	return 1;
}

static struct device *get_fb_dev(void)
{
	struct device *fbdev = NULL;

	/* get the first framebuffer device */
	/* [TODO] Handle properly when there are more than one framebuffer */
	fbdev = class_find_device(fb_class, NULL, NULL, is_device_ok);
	if (NULL == fbdev) {
		pr_debug("ERROR cannot get framebuffer device\n");
		return NULL;
	}
	return fbdev;
}

static struct fb_info *get_fb_info(struct device *fbdev)
{
	struct fb_info *fb_info;

	if (!fbdev->p) {
		pr_debug("ERROR framebuffer device has no private data\n");
		return NULL;
	}

	fb_info = (struct fb_info *)dev_get_drvdata(fbdev);
	if (!fb_info) {
		pr_debug("ERROR framebuffer device has no fb_info\n");
		return NULL;
	}

	return fb_info;
}

static void blank_framebuffer(int getref)
{
	struct device *fbdev = NULL;
	struct fb_info *fb_info;
	struct s3c_fb_win *win;
	struct s3c_fb *sfb;

	fbdev = get_fb_dev();
	if (!fbdev)
		return;

	fb_info = get_fb_info(fbdev);
	if (!fb_info)
		return;

	/*
	 * hold a reference to the dsim device, to prevent it from going into
	 * power management during tui session
	 */
	win = fb_info->par;
	sfb = win->parent;

	if (getref)
		pm_runtime_get_sync(sfb->dev);

	/* blank the framebuffer */
	lock_fb_info(fb_info);
	console_lock();
	fb_info->flags |= FBINFO_MISC_USEREVENT;
	pr_info("%s call fb_blank\n", __func__);
	fb_blank(fb_info, FB_BLANK_POWERDOWN);
	fb_info->flags &= ~FBINFO_MISC_USEREVENT;
	console_unlock();
	unlock_fb_info(fb_info);
	pr_info("%s call s3c_fb_deactivate_vsync\n", __func__);
	s3c_fb_deactivate_vsync(sfb);
}

static void unblank_framebuffer(int releaseref)
{
	struct device *fbdev = NULL;
	struct fb_info *fb_info;
	struct s3c_fb_win *win;
	struct s3c_fb *sfb;

	fbdev = get_fb_dev();
	if (!fbdev)
		return;

	fb_info = get_fb_info(fbdev);
	if (!fb_info)
		return;

	/*
	 * Release the reference we took at the beginning of the TUI session
	 */
	win = fb_info->par;
	sfb = win->parent;

	pr_info("%s call s3c_fb_activate_vsync\n", __func__);
	s3c_fb_activate_vsync(sfb);

	/*
	 * Unblank the framebuffer
	 */
	console_lock();
	fb_info->flags |= FBINFO_MISC_USEREVENT;
	fb_blank(fb_info, FB_BLANK_UNBLANK);
	fb_info->flags &= ~FBINFO_MISC_USEREVENT;
	console_unlock();

	if (releaseref)
		pm_runtime_put_sync(sfb->dev);
}

uint32_t hal_tui_init(void)
{
	/* Allocate memory pool for the framebuffer
	 */
	if (!allocate_tui_memory_pool(&g_tui_mem_pool, TUI_MEMPOOL_SIZE))
		return TUI_DCI_ERR_INTERNAL_ERROR;

	return TUI_DCI_OK;
}

void hal_tui_exit(void)
{
	/* delete memory pool if any */
	if (g_tui_mem_pool.va)
		free_tui_memory_pool(&g_tui_mem_pool);
}

uint32_t hal_tui_alloc(
	struct tui_alloc_buffer_t allocbuffer[MAX_DCI_BUFFER_NUMBER],
	size_t allocsize, uint32_t number)
{
	uint32_t ret = TUI_DCI_ERR_INTERNAL_ERROR;

	if (!allocbuffer) {
		pr_debug("%s(%d): allocbuffer is null\n", __func__, __LINE__);
		return TUI_DCI_ERR_INTERNAL_ERROR;
	}

	pr_debug("%s(%d): Requested size=0x%x x %u chunks\n", __func__,
		 __LINE__, allocsize, number);

	if ((size_t)allocsize == 0) {
		pr_debug("%s(%d): Nothing to allocate\n", __func__, __LINE__);
		return TUI_DCI_OK;
	}

	if (number != 3) {
		pr_debug("%s(%d): Unexpected number of buffers requested\n",
			 __func__, __LINE__);
		return TUI_DCI_ERR_INTERNAL_ERROR;
	}

	if ((size_t)(allocsize*number) <= g_tui_mem_pool.size) {
		/* requested buffer fits in the memory pool */
		unsigned int i;
		for (i = 0; i < number; i++) {
			pr_info("%s(%d): allocbuffer + %d = 0x%p\n", __func__,
				__LINE__, i, allocbuffer+i);
			allocbuffer[i].pa =
				(uint64_t) (g_tui_mem_pool.pa + i * allocsize);
			pr_info("%s(%d): allocated at %llx\n", __func__,
				__LINE__, allocbuffer[i].pa);
		}
		ret = TUI_DCI_OK;
	} else {
		/* requested buffer is bigger than the memory pool, return an
		   error */
		pr_debug("%s(%d): Memory pool too small\n", __func__, __LINE__);
		ret = TUI_DCI_ERR_INTERNAL_ERROR;
	}

	return ret;
}

void hal_tui_free(void)
{
}

uint32_t hal_tui_deactivate(void)
{
	/* Set linux TUI flag */
	trustedui_set_mask(TRUSTEDUI_MODE_TUI_SESSION);
	trustedui_blank_set_counter(0);
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI_FB_BLANK
	blank_framebuffer(1);
	/* TODO-[2014-03-19]-julare01: disabled for Arndale board but this
	 * should be re enabled and put into a HAL */
/*		disable_irq(gpio_to_irq(190)); */
#endif
	trustedui_set_mask(TRUSTEDUI_MODE_VIDEO_SECURED|
			   TRUSTEDUI_MODE_INPUT_SECURED);

	return TUI_DCI_OK;
}

uint32_t hal_tui_activate(void)
{
	/* Protect NWd */
	trustedui_clear_mask(TRUSTEDUI_MODE_VIDEO_SECURED|
			     TRUSTEDUI_MODE_INPUT_SECURED);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI_FB_BLANK
	pr_info("Unblanking\n");
	/* TODO-[2014-03-19]-julare01: disabled for Arndale board but this
	 * should be re enabled and put into a HAL */
/*		enable_irq(gpio_to_irq(190));*/
	unblank_framebuffer(1);
#endif

	/* Clear linux TUI flag */
	trustedui_set_mode(TRUSTEDUI_MODE_OFF);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI_FB_BLANK
	pr_info("Unsetting TUI flag (blank counter=%d)",
		trustedui_blank_get_counter());
	if (0 < trustedui_blank_get_counter())
		blank_framebuffer(0);
#endif

	return TUI_DCI_OK;
}

