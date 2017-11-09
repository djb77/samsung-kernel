/*
 * ALSA SoC - Samsung ABOX driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ABOX_H
#define __ABOX_H

#include <linux/device.h>
#include <linux/irqreturn.h>
#include <sound/soc.h>
#include <sound/samsung/abox_ipc.h>

/**
 * abox irq handler type definition
 * @param[in]	ipc_id		id of ipc
 * @param[in]	dev_id		cookie which would be summitted with irq_handler
 * @param[in]	msg		message data
 * @return	reference irqreturn_t
 */
typedef irqreturn_t (*abox_irq_handler_t)(int ipc_id, void *dev_id,
		ABOX_IPC_MSG *msg);

#ifdef CONFIG_SND_SOC_SAMSUNG_ABOX
/**
 * Check ABOX is on
 * @return		true if A-Box is on, false on otherwise
 */
extern bool abox_is_on(void);

/**
 * Get INT frequency required by ABOX
 * @return		INT frequency in kHz
 */
extern unsigned int abox_get_requiring_int_freq_in_khz(void);

/**
 * Start abox IPC
 * @param[in]	dev		pointer to abox device
 * @param[in]	hw_irq		hardware IRQ number
 * @param[in]	supplement	pointer to data
 * @param[in]	size		size of data which are pointed by supplement
 * @param[in]	atomic		1, if caller context is atomic. 0, if not.
 * @param[in]	sync		1 to wait for ack. 0 if not.
 * @return	error code if any
 */
extern int abox_request_ipc(struct device *dev,
		int hw_irq, const void *supplement,
		size_t size, int atomic, int sync);

/**
 * Start abox IPC
 * @param[in]	dev		pointer to abox device
 * @param[in]	hw_irq		hardware IRQ number
 * @param[in]	supplement	pointer to data
 * @param[in]	size		size of data which are pointed by supplement
 * @param[in]	atomic		1, if caller context is atomic. 0, if not.
 * @param[in]	sync		1 to wait for ack. 0 if not.
 * @return	error code if any
 */
static inline int abox_start_ipc_transaction(struct device *dev,
		int hw_irq, const void *supplement,
		size_t size, int atomic, int sync)
{
	return abox_request_ipc(dev, hw_irq, supplement, size, atomic, sync);
}

/**
 * Register irq handler to abox
 * @param[in]	dev		pointer to abox device
 * @param[in]	ipc_id		id of ipc
 * @param[in]	irq_handler	abox irq handler to register
 * @param[in]	dev_id		cookie which would be summitted with irq_handler
 * @return	error code if any
 */
extern int abox_register_irq_handler(struct device *dev, int ipc_id,
		abox_irq_handler_t irq_handler, void *dev_id);

/**
 * UAIF/DSIF hw params fixup helper
 * @param[in]	rtd	snd_soc_pcm_runtime
 * @param[out]	params	snd_pcm_hw_params
 * @return		error code if any
 */
extern int abox_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params);
#else /* !CONFIG_SND_SOC_SAMSUNG_ABOX */
static inline bool abox_is_on(void) { return false; }
static inline unsigned int abox_get_requiring_int_freq_in_khz(void)
{ return 0; }
static inline int abox_request_ipc(struct device *dev,
		int hw_irq, const void *supplement,
		size_t size, int atomic, int sync) { return -ENODEV; }
static inline int abox_start_ipc_transaction(struct device *dev,
		int hw_irq, const void *supplement,
		size_t size, int atomic, int sync)
{
	return abox_request_ipc(dev, hw_irq, supplement, size, atomic, sync);
}
static inline int abox_register_irq_handler(struct device *dev, int ipc_id,
		abox_irq_handler_t irq_handler, void *dev_id)
{ return -ENODEV; }
static inline int abox_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params) { return -ENODEV; }
#endif /* !CONFIG_SND_SOC_SAMSUNG_ABOX */

#endif /* __ABOX_H */

