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

#define EX1_DRAM_IN_BASE_ADDR_OFS	0x00000000
#define EX1_DRAM_OUT_BASE_ADDR_OFS	0x01000000

/* EXAMPLE 1: multiple sub chains and advanced PU's connection.
 *
 * TASK: 3 vertices (start -> 1 Process -> end).
 * PROCESS 1: the process has 2 HW sub-chains. on of the second chain inputs is
 * chain 1 output. (Chain 1 implement 2' compliment)
 * SUBCHAIN 1: 3 PU (DMA IN -> SALB (inv) -> SALB (+1) -> DMA OUT)
 *	DATA:	     0xFFFB            0x0004         0x0005
 * SUBCHAIN 2: 5 PU (DMA IN -> SALB (sub) -> CALB (invert) -> DMA OUT).
 *		     DMA_IN ->
 *                   0x0003             0x0002           0xFFFD
 * 2+3 IO (fixed DRAM).
 */

const __u32 get_task_ds_size_params_ex1[4] = {3, 2, 9, 0};
const __u32 subchains_for_vertices_ex1[3] = {0, 2, 0};
const __u32 pus_for_subchains_ex1[2] = {4, 5};

#define EX1_IN_WIDTH	32
#define EX1_IN_HEIGHT	32
#define EX1_PIXEL_BYTES	2
#define EX1_IMG_DRAM_SIZE	(EX1_IN_HEIGHT * EX1_IN_WIDTH * EX1_PIXEL_BYTES)
#define EX1_OUT_DRAM_SIZE	EX1_IMG_DRAM_SIZE
#define EX1_DRAM_IN_SIZE	(2 * EX1_IMG_DRAM_SIZE) /* two images */

#define EX1_EXT_IN_MEM_ADDRESSES	2
#define EX1_EXT_TMP_MEM_ADDRESSES	1
#define EX1_EXT_OUT_MEM_ADDRESSES	1
const __u32 EX1_EXT_IN_MEM_ADDR[] = {
	EX1_DRAM_IN_BASE_ADDR_OFS,
	EX1_DRAM_IN_BASE_ADDR_OFS + 1 * EX1_IMG_DRAM_SIZE};

const __u32 EX1_EXT_TMP_MEM_ADDR[] = {
	EX1_DRAM_IN_BASE_ADDR_OFS + 2 * EX1_IMG_DRAM_SIZE};

const __u32 EX1_EXT_OUT_MEM_ADDR[] = {
	EX1_DRAM_OUT_BASE_ADDR_OFS};


#define SC1_IN0_DATA	-5
#define SC2_IN1_DATA	3
#define EXP_OUT_DATA	0xFFFD

enum ex1_process1_sizes {
	EX1_P1_SIZES_IN_OUT = 0,
	EX1_P1_SIZES_ALL
};

static __u32 vpu_example1_get_task_ds_size(void)
{
	return vpu_translator_get_task_ds_size(
		get_task_ds_size_params_ex1[0],
		get_task_ds_size_params_ex1[1],
		get_task_ds_size_params_ex1[2],
		get_task_ds_size_params_ex1[3]
		);
}

static __s32 vpu_example1_create_task_ds(
	struct vpul_task *task, __u32 size_allocated)
{
	return vpu_translator_create_task_ds_from_array(task, size_allocated,
	get_task_ds_size_params_ex1[0],
	get_task_ds_size_params_ex1[1],
	get_task_ds_size_params_ex1[2],
	get_task_ds_size_params_ex1[3],
	subchains_for_vertices_ex1,
	pus_for_subchains_ex1
	);
}

static void example1_fill_task_data_structure(struct vpul_task *task)
{
	struct vpul_vertex *vertices = fst_vtx_ptr(task);
	struct vpul_vertex *vtx;
	struct vpul_subchain *sc;
	struct vpul_pu *pu;

	/* task */
	task->id = 3;
	task->priority = 1;

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
	vtx->proc.io.n_sizes_op = EX1_P1_SIZES_ALL;
	/* input/output types */
	vtx->proc.io.n_inout_types = 4;
	vtx->proc.io.inout_types[0].is_dynamic = 0;
	vtx->proc.io.inout_types[0].n_insets_per_dynamic_roi = 0; /* fixed ROI */
	vtx->proc.io.inout_types[0].roi_index = 0;
	vtx->proc.io.inout_types[1].is_dynamic = 0;
	vtx->proc.io.inout_types[1].n_insets_per_dynamic_roi = 0; /* fixed ROI */
	vtx->proc.io.inout_types[1].roi_index = 1;
	vtx->proc.io.inout_types[2].is_dynamic = 0;
	vtx->proc.io.inout_types[2].n_insets_per_dynamic_roi = 0; /* fixed ROI */
	vtx->proc.io.inout_types[2].roi_index = 2;
	vtx->proc.io.inout_types[3].is_dynamic = 0;
	vtx->proc.io.inout_types[3].n_insets_per_dynamic_roi = 0; /* fixed ROI */
	vtx->proc.io.inout_types[3].roi_index = 3;

	/* ROIs */
	vtx->proc.io.n_fixed_map_roi = 4;
	vtx->proc.io.n_dynamic_map_rois = 0;
	vtx->proc.io.fixed_map_roi[0].memmap_idx = 0;
	vtx->proc.io.fixed_map_roi[0].use_roi = 0;
	vtx->proc.io.fixed_map_roi[1].memmap_idx = 1;
	vtx->proc.io.fixed_map_roi[1].use_roi = 0;
	vtx->proc.io.fixed_map_roi[2].memmap_idx = 2;
	vtx->proc.io.fixed_map_roi[2].use_roi = 0;
	vtx->proc.io.fixed_map_roi[3].memmap_idx = 3;
	vtx->proc.io.fixed_map_roi[3].use_roi = 0;

	/* memory IO: two for input, two for output */
	task->n_external_mem_addresses = 4;
	task->external_mem_addr[0] = EX1_EXT_IN_MEM_ADDR[0];
	task->external_mem_addr[1] = EX1_EXT_IN_MEM_ADDR[1];
	task->external_mem_addr[2] = EX1_EXT_TMP_MEM_ADDR[0];
	task->external_mem_addr[3] = EX1_EXT_OUT_MEM_ADDR[0];
	task->n_memmap_desc = 4;
	task->memmap_desc[0].mtype = VPUL_MEM_EXTERNAL;
	task->memmap_desc[0].index = 0;
	task->memmap_desc[0].image_sizes.width = EX1_IN_HEIGHT;
	task->memmap_desc[0].image_sizes.height = EX1_IN_WIDTH;
	task->memmap_desc[0].image_sizes.line_offset =
		EX1_IN_WIDTH * EX1_PIXEL_BYTES;
	task->memmap_desc[0].image_sizes.pixel_bytes = EX1_PIXEL_BYTES;
	task->memmap_desc[1] = task->memmap_desc[0];
	task->memmap_desc[1].index = 1;
	task->memmap_desc[2] = task->memmap_desc[0];
	task->memmap_desc[2].index = 2;
	task->memmap_desc[3] = task->memmap_desc[0];
	task->memmap_desc[3].index = 3;

	/* sizes */
	vtx->proc.io.n_sizes_op = 1;
	vtx->proc.io.sizes[0].op_ind = 0;
	vtx->proc.io.sizes[0].type = VPUL_SIZEOP_INOUT;

	vtx->proc.io.n_tiles = 0;
	vtx->proc.io.n_presets_map_desc = 0;




	vtx->proc.io.n_insets = 1;

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
	pu->in_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->params.dma.inout_index = 0;
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
	pu->in_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX1_P1_SIZES_IN_OUT;
	memset(&pu->params.salb, 0, sizeof(struct vpul_pu_salb));
	pu->params.salb.bits_in0 = 1;
	pu->params.salb.bits_in1 = 1;
	pu->params.salb.bits_out0 = 1;
	pu->params.salb.operation_code = 7; /* Out0 = Mask ? ~ SI : SI */
	pu++;
	/* SALB: u16:   +1 */
	pu->instance = VPU_PU_SALB1;
	pu->op_type = VPUL_OP_FULL_SALB;
	pu->n_in_connect = 1;
	pu->in_connect[0].pu_idx = 1;
	pu->in_connect[0].s_pu_out_idx = 0;
	pu->n_out_connect = 1;



	pu->n_mprbs = 0;
	pu->in_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX1_P1_SIZES_IN_OUT;
	memset(&pu->params.salb, 0, sizeof(struct vpul_pu_salb));
	pu->params.salb.bits_in0 = 1;
	pu->params.salb.bits_in1 = 1;
	pu->params.salb.bits_out0 = 1;
	pu->params.salb.operation_code = 9; /* Out0=Mask?(SI + ValMed):SI */
	pu->params.salb.val_med_filler = 1; /*puls 1*/
	pu++;
	/* DMA out */
	pu->instance = VPU_PU_DMAOT0;
	pu->op_type = VPUL_OP_DMA;
	pu->n_in_connect = 1;
	pu->in_connect[0].pu_idx = 2;
	pu->in_connect[0].s_pu_out_idx = 0;
	pu->n_out_connect = 0;
	pu->n_mprbs = 0;
	pu->in_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->params.dma.inout_index = 2;
	pu->params.dma.offset_lines_inc = 0;/*TBD*/
	pu++;

	/****** subchain 2 *****/
	sc[1].stype = VPUL_SUB_CH_HW;
	sc[1].id = 0x1239;
	/* DMA In */
	pu->instance = VPU_PU_DMAIN0;
	pu->op_type = VPUL_OP_DMA;
	pu->n_in_connect = 0;
	pu->n_out_connect = 1;



	pu->n_mprbs = 0;
	pu->in_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->params.dma.inout_index = 2;
	pu->params.dma.offset_lines_inc = 0;/*TBD*/
	pu++;
	/* DMA In */
	pu->instance = VPU_PU_DMAIN_MNM0;
	pu->op_type = VPUL_OP_DMA;
	pu->n_in_connect = 0;
	pu->n_out_connect = 1;



	pu->n_mprbs = 0;
	pu->in_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->params.dma.inout_index = 1;
	pu->params.dma.offset_lines_inc = 0;/*TBD*/
	pu++;
	/* SALB: u16:  sub */
	pu->instance = VPU_PU_SALB2;
	pu->op_type = VPUL_OP_FULL_SALB;
	pu->n_in_connect = 2;
	pu->in_connect[0].pu_idx = 0;
	pu->in_connect[0].s_pu_out_idx = 0;
	pu->in_connect[1].pu_idx = 1;
	pu->in_connect[1].s_pu_out_idx = 0;
	pu->n_out_connect = 1;



	pu->n_mprbs = 0;
	pu->in_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX1_P1_SIZES_IN_OUT;
	memset(&pu->params.salb, 0, sizeof(struct vpul_pu_salb));
	pu->params.salb.input_enable = 0x3;
	pu->params.salb.bits_in0 = 1;
	pu->params.salb.bits_in1 = 1;
	pu->params.salb.bits_out0 = 1;
	pu->params.salb.operation_code = 0; /* Out0 = Mask ? (In0 - In1) :SI */
	pu++;
	/* CALB: u16: invert */
	pu->instance = VPU_PU_CALB0;
	pu->op_type = VPUL_OP_FULL_CALB;
	pu->n_in_connect = 1;
	pu->in_connect[0].pu_idx = 2;
	pu->in_connect[0].s_pu_out_idx = 0;
	pu->n_out_connect = 1;



	pu->n_mprbs = 0;
	pu->in_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX1_P1_SIZES_IN_OUT;
	memset(&pu->params.calb, 0, sizeof(struct vpul_pu_calb));
	pu->params.calb.bits_in0 = 1;
	pu->params.calb.bits_in1 = 1;
	pu->params.calb.bits_out0 = 1;
	pu->params.calb.operation_code = 7; /* Out0 = Mask ? ~SI : SI*/
	pu++;
	/* DMA out */
	pu->instance = VPU_PU_DMAOT0;
	pu->op_type = VPUL_OP_DMA;
	pu->n_in_connect = 1;
	pu->in_connect[0].pu_idx = 3;
	pu->in_connect[0].s_pu_out_idx = 0;
	pu->n_out_connect = 0;
	pu->n_mprbs = 0;
	pu->in_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->out_size_idx = EX1_P1_SIZES_IN_OUT;
	pu->params.dma.inout_index = 3;
	pu->params.dma.offset_lines_inc = 0;/*TBD*/
	pu++;

	/* Vertex 3 - End */
	vtx = &vertices[2];
	vtx->vtype = VPUL_VERTEXT_END;
	vtx->n_out_edges = 0;
	vtx->loop.type = VPU_LOOP_NONE;
}

struct vpul_task * make_example1_task(int *size)
{
	int status;
	struct vpul_task *task = NULL;
	__u32 needed_size;

	/* Create Data structure */
	needed_size  = vpu_example1_get_task_ds_size();
	task = malloc(needed_size + 4);
	if (!task) {
		printf("fail allocate task");
		return NULL;
	}

	status = vpu_example1_create_task_ds(task, needed_size);
	if (status != VPU_STATUS_SUCCESS) {
		printf("vpu_translator_create_task_ds fail (%d)\n", status);
		return NULL;
	}

	/* Driver fill task DS */
	example1_fill_task_data_structure(task);

	*size = needed_size + 4;
	return task;
}