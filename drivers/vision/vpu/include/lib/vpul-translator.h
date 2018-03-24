/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if !defined(__VPU_TRANSLATOR__)
#define __VPU_TRANSLATOR__

#include "vpul-ds.h"

/*! \brief
* define maximum filters in the task
*/
#define MAX_POSSIBLE_FILTERS_IN_PROCESS  (20)


/** \addtogroup VPUL-Translator
 *  @{
 */



/*! \brief this function returns a pointer to vertex referencing a 3DNN process base
 * specified at input
 * \return NULL if no such vertex found
 * if more than 1 vertex found, returns the one with largest number of subchains
 * inputs : task - pointer to TDS
 *          proc_base_3dnn_idx - index of specified 3DNN process base
 */
struct vpul_vertex *vertex_referencing_this_3dnn_proc_base(const struct vpul_task *task,
								__u32 proc_base_3dnn_idx);

/**
  * \brief Initialize Core FW memory parameters and MailBox settings
  * \note this command must be written to HOST normal commands MailBox.
  * \param inf Core init command settings.
  * \param mailbox_command_size The actual size in bytes of the MailBox command.
  * \param mailbox_command A pointer to memory location where MailBox command
  *			will be returned. If it's NULL, the function will return
  *			the MailBox command size in mailbox_command_size.
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_init_core(
	const struct vpul_core_init_command_params *inf,
	size_t *mailbox_command_size,
	void *mailbox_command
	);

/**
  * vpu_translator_reallocate_mbox() - reallocation for Mbox sizes.
  * and MailBox settings
  * \note this command must be written to HOST normal commands MailBox.
  * \param realloc_info Mbox reallocation information-new size for normal host
  * mail box in halfword (16bit)
  * \param mailbox_command_size The actual size in bytes of the MailBox
  *		  command.
  * \param mailbox_command A pointer to memory location where MailBox command
  *	 will be returned. If it's NULL, the function will return the
  *	 MailBox command size in mailbox_command_size.
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_reallocate_mbox(
	struct vpul_mbox_reallocate *realloc_info,
	size_t *mailbox_command_size,
	void *mailbox_command
	);


/**
  * \brief Initialize task data structure
  * \param task pointer to memory allocated for task data structure of
  *			size received by calling vpu_translator_get_task_ds_size
  * \param size_allocated If 0 indicates method will perform allocation.
  *		If not will be verified to match needed allocation size
  * \param t_num_of_vertices total number of vertices in task
  * \param t_num_of_subchains total number of sub chains (in all processes)
  * \param t_num_of_pus total number of processing units
  * \param t_num_of_updatable_pus total number of processing units that it's
  *			parameters are updated during invoke command.
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_create_task_ds(
	struct vpul_task *task,
	__u32 size_allocated,
	__u32 t_num_of_vertices,
	__u32 t_num_of_subchains,
	__u32 t_num_of_pus,
	__u32 t_num_of_updatable_pus,
	...);

/**
  * vpu_translator_task_create() - Prepare "Task Create" MailBox command.
  * \param task A Pointer to Host Task data structure.
  * \param mailbox_command_size The actual size in bytes of the MailBox command.
  * \param mailbox_command A pointer to memory location where MailBox
  *			command will be returned. If it's NULL, the function will
  *			return the MailBox command size in mailbox_command_size.
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_task_create(
	struct vpul_task *task,
	size_t *mailbox_command_size,
	void *mailbox_command
	);

/**
  * vpu_translator_task_create_w_mem_mark() -
  * Prepare "Task Create" MailBox command. With option to specify external
  * buffers that need to be updated on each invocation.
  * \param task A Pointer to Host Task data structure.
  * \param invoke_mem_mark A pointer to struct with marking
  *			about which external should be update on invoke
  * \param mailbox_command_size The actual size in bytes of the MailBox command.
  * \param mailbox_command A pointer to memory location where MailBox
  *			command will be returned. If it's NULL, the function will
  *			return the MailBox command size in mailbox_command_size.
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_task_create_w_mem_mark(
struct vpul_task *task,
	const struct vpu_mark_for_invoke_ext_mem_vec_ds *invoke_mem_mark,
	size_t *mailbox_command_size,
	void *mailbox_command
	);

/**
  * vpu_translator_task_allocate() - Prepare "Task Allocation" MailBox command.
  * \param task Pointer to Host Task data structure.
  * \param number_of_slots number of slots for allocation.
  * \param mailbox_command_size The actual size in bytes of the MailBox command.
  * \param mailbox_command A pointer to memory location where MailBox
  *			command will be returned. If it's NULL, the function will
  *			return the MailBox command size in mailbox_command_size.
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_task_allocate(
	const struct vpul_task *task,
	__u32 number_of_slots,
	size_t *mailbox_command_size,
	void *mailbox_command
	);

/**
  * vpu_translator_task_update_parameters() - Prepare "Task parameters update"
  *						MailBox command.
  * \param task Pointer to Host Task data structure.
  * \param number_of_parameters_to_update Number on entries on parameters vector
  * \param update_parameters_vector Parameters vector
  * \param mailbox_command_size The actual size in bytes of the MailBox command.
  * \param mailbox_command A pointer to memory location where MailBox
  *			command will be returned. If it's NULL, the function will
  *			return the MailBox command size in mailbox_command_size.
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_task_update_parameters(
	const struct vpul_task *task,
	__u32 number_of_parameters_to_update,
	struct vpu_update_parameters_vector_ds *update_parameters_vector,
	size_t *mailbox_command_size,
	void *mailbox_command
	);

/**
  * vpu_translator_task_invoke() - Prepare "Task Invocation" MailBox command.
  * \param task Pointer to Host Task data structure.
  * \param invoaction_id Unique (per task) invocation ID.
  * Will be used on Core reports. invocation ID must be != 0.
  * \param const union vpul_pu_parameters *invoke_pu_params_vector, invoke params
  * \param mailbox_command_size The actual size in bytes of the MailBox command.
  * \param mailbox_command A pointer to memory location where MailBox
  *			command will be returned. If it's NULL, the function will
  *			return the MailBox command size in mailbox_command_size.
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_task_invoke(
	const struct vpul_task *task,
	__u32 invoaction_id,
	const union vpul_pu_parameters *invoke_pu_params_vector,
	const struct vpu_invoke_external_mem_vector_ds *invoke_memories_vector,
	size_t *mailbox_command_size,
	void *mailbox_command
	);

/**
  * vpu_translator_task_destroy() - Prepare "Task Destroy" MailBox command.
  * \param task Pointer to Host Task data structure.
  * \param mailbox_command_size The actual size in bytes of the MailBox command.
  * \param mailbox_command A pointer to memory location where MailBox
  *			command will be returned. If it's NULL, the function will
  *			return the MailBox command size in mailbox_command_size.
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_task_destroy(
	const struct vpul_task *task,
	size_t *mailbox_command_size,
	void *mailbox_command
	);

/**
  * vpu_translator_task_abort() - Prepare "Task Abort" MailBox command.
  * \param task Pointer to Host Task data structure.
  * \param mailbox_command_size The actual size in bytes of the MailBox command.
  * \param mailbox_command A pointer to memory location where MailBox
  *			command will be returned. If it's NULL, the function will
  *			return the MailBox command size in mailbox_command_size.
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_task_abort(
	const struct vpul_task *task,
	size_t *mailbox_command_size,
	void *mailbox_command
	);

/**
  * vpu_translator_task_free() - Prepare "Free Task" MailBox command.
  * \param task Pointer to Host Task data structure.
  * \param mailbox_command_size The actual size in bytes of the MailBox command.
  * \param  will mailbox_command A pointer to memory location where MailBox
  *			command will be returned. If it's NULL, the function will
  *			return the MailBox command size in mailbox_command_size.
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_task_free(
	const struct vpul_task *task,
	size_t *mailbox_command_size,
	void *mailbox_command
	);

/**
* /// generate_and_wr_invoke_parms_vector - creates invoke words vector from given pu params.
	* \param task Pointer to Host Task data structure.
	* \param vpul_pu_parameters Pointer given pus params.
	* \param mb_cp Pointer to MB.
	* \pre The task was destroyed/free successfully
	* \return 0 on success otherwise none zero
	*/

__u32 generate_and_wr_invoke_parms_vector(
	const struct vpul_task *task,
	const union vpul_pu_parameters *invoke_pu_params_vector,
	__u8 *mb_cp
	);


/** @}*/

#endif
