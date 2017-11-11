#include <stdio.h>
#include <stdarg.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>

#include <ctype.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <vs4l.h>
#include <lib/vpul-errno.h>
#include "test-client.h"
#include "test-list.h"
#include "test-util.h"

#define EX0_DRAM_IN_BASE_ADDR_OFS	0x00000000
#define EX0_DRAM_OUT_BASE_ADDR_OFS	0x01000000

/* EXAMPLE 0: This is an example for the most basic task.
 *
 * TASK:  vertices (start -> 1 Process -> end).
 * PROCESS 1: the process has 1 HW sub-chains. The data is read and written
 * from/to external RAM. Fixed ROIs.
 * SUBCHAIN 1: 3 PU (DMA IN -> SALB -> DMA OUT).
 * The SALB configure as inverter for unsigned 16 streams (1 input, 1 output)
 */
const __u32 get_task_ds_size_params_ex0[4] = /* added 1 pu param */
						{3,	/* Num of Vets */
						1,	/* Num of SCs  */
						3,	/* Num of PUs  */
						1};	/* Num of invk prms  */
const __u32 create_task_ds_ext_params_ex0[4] = {0, 1, 0,   3};

const __u32 create_task_ds_invoke_params_locatios_ex0[6*3] =
			{
			1,0,1, // 1 prm,  2nd vertix [no PU's on start Vtx], first SC, first PU.
			1,0,1, // 3 prms, will not be used if # of invoke params < 2
			1,0,2, // 1 prm,  will not be used if # of invoke params < 3
			1,1,0, // not valid for example 0, no such SC.
			2,0,0, // not valid for example 0, no such vtx.
			1,1,1, // not valid - not ordered .
			};

#define EX0_IN_WIDTH	16
#define EX0_IN_HEIGHT	16
#define EX0_PIXEL_BYTES	2
#define EX0_IN_DRAM_SIZE \
	(EX0_IN_HEIGHT * EX0_IN_WIDTH * EX0_PIXEL_BYTES)
#define EX0_OUT_DRAM_SIZE	EX0_IN_DRAM_SIZE

#define EX0_EXT_MEM_ADDRESSES	2
const __u32 EX0_EXT_MEM_ADDR[EX0_EXT_MEM_ADDRESSES] = {
	EX0_DRAM_IN_BASE_ADDR_OFS, EX0_DRAM_OUT_BASE_ADDR_OFS};
const __u32 EX0_EXT_INPUT_MEM_SIZE[1] =	{EX0_IN_DRAM_SIZE};

enum process1_sizes {
	EX0_P1_SIZES_IN_OUT = 0,
	EX0_P1_SIZES_ALL
};

static __u32 vpu_example0_get_task_ds_size(void)
{
	return vpu_translator_get_task_ds_size(
		get_task_ds_size_params_ex0[0],
		get_task_ds_size_params_ex0[1],
		get_task_ds_size_params_ex0[2],
		0);
}

__u32 vpu_example0_get_task_ds_size_with_invk_prms(void)
{
	return vpu_translator_get_task_ds_size(
		get_task_ds_size_params_ex0[0],
		get_task_ds_size_params_ex0[1],
		get_task_ds_size_params_ex0[2],
		get_task_ds_size_params_ex0[3]); // sync with Example0InvkPrmsCreateTask
}

__s32 vpu_example0_create_task_ds(
	struct vpul_task *task, __u32 size_allocated)
{
	return vpu_translator_create_task_ds(task, size_allocated,
	get_task_ds_size_params_ex0[0],
	get_task_ds_size_params_ex0[1],
	get_task_ds_size_params_ex0[2],
	0,
	create_task_ds_ext_params_ex0[0],
	create_task_ds_ext_params_ex0[1],
	create_task_ds_ext_params_ex0[2],
	create_task_ds_ext_params_ex0[3]);
}

__s32 vpu_example0_create_task_ds_with_invk_prms(
struct vpul_task *task, __u32 size_allocated)
{
	return vpu_translator_create_task_ds(task, size_allocated,
		get_task_ds_size_params_ex0[0],
		get_task_ds_size_params_ex0[1],
		get_task_ds_size_params_ex0[2],
		get_task_ds_size_params_ex0[3],
		create_task_ds_ext_params_ex0[0],
		create_task_ds_ext_params_ex0[1],
		create_task_ds_ext_params_ex0[2],
		create_task_ds_ext_params_ex0[3]);
}

void example0_fill_pu_params_for_invoke(union vpul_pu_parameters* pu_prm_var)
{
	memset(&pu_prm_var[0].salb, 0, sizeof(struct vpul_pu_salb));
	memset(&pu_prm_var[1].dma, 0, sizeof(struct vpul_pu_dma));


	pu_prm_var[1].salb.bits_in0 = 1;
	pu_prm_var[1].salb.bits_in1 = 1;
	pu_prm_var[1].salb.bits_out0 = 1;
	pu_prm_var[1].salb.operation_code = 7; /* Out0 = Mask ? ~ SI : SI */

	// copy DMA part
	pu_prm_var[0].dma.inout_index = 0;
	pu_prm_var[0].dma.offset_lines_inc = 0;

	return;
}

static void example0_fill_task_data_structure(struct vpul_task *task)
{
	struct vpul_vertex *vertices = fst_vtx_ptr(task);
	struct vpul_vertex *vtx;
	struct vpul_subchain *sc;
	struct vpul_pu *pu;
	struct vpul_pu_location *updateble_pu_location;
	__u16 cntr = 0;
	/* task */
	task->id = 2;
	task->priority = 1;

	/* memory IO: one for input, one for output */

	task->n_external_mem_addresses = EX0_EXT_MEM_ADDRESSES;
	task->external_mem_addr[0] = EX0_EXT_MEM_ADDR[0];
	task->external_mem_addr[1] = EX0_EXT_MEM_ADDR[1];
	task->n_memmap_desc = 2;
	task->memmap_desc[0].mtype = VPUL_MEM_EXTERNAL;
	task->memmap_desc[0].index = 0;
	task->memmap_desc[0].image_sizes.width = EX0_IN_HEIGHT;
	task->memmap_desc[0].image_sizes.height = EX0_IN_WIDTH;
	task->memmap_desc[0].image_sizes.line_offset =
		EX0_IN_WIDTH * EX0_PIXEL_BYTES;
	task->memmap_desc[0].image_sizes.pixel_bytes = EX0_PIXEL_BYTES;
	task->memmap_desc[1] = task->memmap_desc[0];
	task->memmap_desc[1].index = 1;

	/* vertex 1 - Start */
	vtx = &vertices[0];
	vtx->vtype = VPUL_VERTEXT_START;
	vtx->n_out_edges = 1;
		vtx->out_edges[0].dst_vtx_idx = 1;
	vtx->loop.type = VPU_LOOP_NONE;

	/* Vertex 2 - Process */
	vtx = &vertices[1];
	vtx->vtype = VPUL_VERTEXT_PROC;
	/* output edges */
	vtx->n_out_edges = 1;
		vtx->out_edges[0].dst_vtx_idx = 2;
	/* loop */
	vtx->loop.type = VPU_LOOP_NONE;

	/* set process */
	vtx->proc.io.n_sizes_op = EX0_P1_SIZES_ALL;
	/* input/output types */
	vtx->proc.io.n_inout_types = 2;
	vtx->proc.io.inout_types[0].is_dynamic = 0;
	vtx->proc.io.inout_types[0].n_insets_per_dynamic_roi = 0; /* fixed ROI */
	vtx->proc.io.inout_types[0].roi_index = 0;
	vtx->proc.io.inout_types[1].is_dynamic = 0;
	vtx->proc.io.inout_types[1].n_insets_per_dynamic_roi = 0; /* fixed ROI */
	vtx->proc.io.inout_types[1].roi_index = 1;
	/* ROIs */
	vtx->proc.io.n_fixed_map_roi = 2;
	vtx->proc.io.n_dynamic_map_rois = 0;
	vtx->proc.io.fixed_map_roi[0].memmap_idx = 0;
	vtx->proc.io.fixed_map_roi[0].use_roi = 0;
	vtx->proc.io.fixed_map_roi[1].memmap_idx = 1;
	vtx->proc.io.fixed_map_roi[1].use_roi = 0;
	/* sizes */
	vtx->proc.io.n_sizes_op = 1;
	vtx->proc.io.sizes[0].type = VPUL_SIZEOP_INOUT;
	vtx->proc.io.sizes[0].op_ind = 0;
	vtx->proc.io.sizes[0].type = VPUL_SIZEOP_INOUT;

	vtx->proc.io.n_tiles = 0;
	vtx->proc.io.n_presets_map_desc = 0;




	/* process input loops */
	vtx->proc.io.n_invocations_per_input_tile = 1;

	/* set sub-chains */
	sc = fst_vtx_sc_ptr(task, vtx);
	sc[0].stype = VPUL_SUB_CH_HW;
	sc[0].id = 0x1234;
	/* set pus */
	pu = fst_pu_ptr(task);
	/****** subchain 1 *****/
	/* DMA In */
	pu->instance = VPU_PU_DMAIN0;
	pu->op_type = VPUL_OP_DMA;
	pu->n_in_connect = 0;
	pu->n_out_connect = 1;
	pu->n_mprbs = 0;
	pu->in_size_idx = EX0_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX0_P1_SIZES_IN_OUT;
	pu->params.dma.inout_index = 0;/*TBD*/
	pu->params.dma.offset_lines_inc = 0;/*TBD*/
	pu++;
	/* SALB: u16:  OUT = !IN */
	pu->instance = VPU_PU_SALB0;
	pu->op_type = VPUL_OP_FULL_SALB;
	pu->n_in_connect = 1;
	pu->in_connect[0].pu_idx = 0;
	pu->in_connect[0].s_pu_out_idx = 0;
	pu->n_out_connect = 1;

	pu->n_mprbs = 0;
	pu->in_size_idx = EX0_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX0_P1_SIZES_IN_OUT;
	memset(&pu->params.salb, 0, sizeof(struct vpul_pu_salb));

	pu->params.salb.bits_in0 = 1;
	pu->params.salb.bits_in1 = 1;
	pu->params.salb.bits_out0 = 1;
	pu->params.salb.operation_code = 7; /* Out0 = Mask ? ~ SI : SI */
	pu++;
	/* DMA out */
	pu->instance = VPU_PU_DMAOT_WIDE0;
	pu->op_type = VPUL_OP_DMA;
	pu->n_in_connect = 1;
	pu->in_connect[0].pu_idx = 1;
	pu->in_connect[0].s_pu_out_idx = 0;
	pu->n_out_connect = 0;
	pu->n_mprbs = 0;
	pu->in_size_idx = EX0_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX0_P1_SIZES_IN_OUT;
	pu->params.dma.inout_index = 1;/*TBD*/
	pu->params.dma.offset_lines_inc = 0;/*TBD*/
	pu++;
	vtx->proc.io.n_insets = 1;
	/* set updatable PU's offset on TDS */
	updateble_pu_location = fst_updateble_pu_location_ptr(task);
	for ( cntr = 0;
		cntr<task->t_num_of_pu_params_on_invoke;
		cntr++)
		{
			updateble_pu_location->vtx_idx			=
				create_task_ds_invoke_params_locatios_ex0[3*cntr + 0];
			updateble_pu_location->sc_idx_in_vtx	=
				create_task_ds_invoke_params_locatios_ex0[3*cntr + 1];
			updateble_pu_location->pu_idx_in_sc			=
				create_task_ds_invoke_params_locatios_ex0[3*cntr + 2];
			updateble_pu_location++;
		}

	/* Vertex 3 - End */
	vtx = &vertices[2];
	vtx->vtype = VPUL_VERTEXT_END;
	vtx->n_out_edges = 0;
		vtx->loop.type = VPU_LOOP_NONE;
}

struct vpul_task * make_example0_task(int *size)
{
	int status;
	struct vpul_task *task = NULL;
	__u32 needed_size;

	/* Create Data structure */
	needed_size  = vpu_example0_get_task_ds_size();
	task = malloc(needed_size + 4);
	if (!task) {
		printf("fail allocate task");
		return NULL;
	}

	status = vpu_example0_create_task_ds(task, needed_size);
	if (status != VPU_STATUS_SUCCESS) {
		printf("vpu_translator_create_task_ds fail (%d)\n", status);
		return NULL;
	}

	/* Driver fill task DS */
	example0_fill_task_data_structure(task);

	*size = needed_size + 4;
	return task;
}
