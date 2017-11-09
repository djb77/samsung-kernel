/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/bug.h>

#include "lib/vpul-def.h"
#include "lib/vpul-errno.h"
#include "lib/vpul-gen.h"
#include "lib/vpul-translator.h"
#include "lib/vpul-hwmapper.h"
#include "lib/vpu-fwif-commands.h"
#include "vpul_ds_internal.h"

#define BUG_ON_NOT_IMPLEMENTED_FUNCT(A)                BUG_ON(A)

#define VPU_MAX_MB_CREATE_TASK_PARAM_SIZE	(8192)/* TODO: remove */
#define DEFAULT_CNN_SCALER			(0x3C00)

/* The following functions perform sanity check for task data structure,
 * It might be called only on debug modes
 */
/* help macros */
#define VTXIDX(_vtxb, _vtx)\
	(__u32)((struct vpul_vertex *)(_vtx) - (struct vpul_vertex *)(_vtxb))

#define VTXPTR(_vtxb, _idx)\
	(struct vpul_vertex *)((_vtxb) + (_idx))

#define NxtVtxPtr(_vtxb, _vtx, _outedg)\
	VTXPTR(_vtxb, (_vtx)->out_edges[(_outedg)].dst_vtx_idx)

#define PAD_U8_PTR_TO_ALIGN(a) \
	*(a)=(((ulong)(a))%2!=0)? (0xAA): *(a)

static const enum VPUI_SizesOpType sizeop_type_lut[VPUL_SIZEOP_NUM] = {
	VPUI_SIZEOP_INOUT,
	VPUI_SIZEOP_FIX,
	VPUI_SIZEOP_FORCE_CROP,
	VPUI_SIZEOP_CROP_ON_EDGES_TILES,
	VPUI_SIZEOP_SCALE
};

static const enum VPUI_3DXYSizesOpType sizeop_3dxy_type_lut[VPUL_3DXY_SIZEOP_NUM] = {
	VPUI_3DXY_SIZEOP_INOUT,
	VPUI_3DXY_SIZEOP_ZIN_TO_XY,
	VPUI_3DXY_SIZEOP_ZOUT_TO_XY,
	VPUI_3DXY_SIZEOP_CROP,
	VPUI_3DXY_SIZEOP_SCALE
};



static void set_3dnn_layers_descr_for_proc_base(const struct vpul_task *task,
						const struct VPUI_TaskCreate *mb_tc,
						const struct vpul_3dnn_process_base *proc_base,
						struct mb_ptrs *mbp, __u8 *mb_cp);


static __u32 count_proc_int_ram_desc(
			const struct vpul_task *task,
			const struct vpul_vertex *vtx );

/* Force the address to be multiply of 2 */
static __u32 m2size(__u32 val)
{
	return (val + 1) & ~1;
}

/* check message size */
static __s32 check_size(__u32 msg_size, size_t *command_size, void *cmd)
{
	if (command_size == NULL) {
		VPU_DEBG("mailbox_command_size is NULL.\n");
		return VPU_STATUS_BAD_PARAMS;
	} else if (cmd == NULL) {
		*command_size = msg_size;
		return VPU_STATUS_SUCCESS;
	} else if (*command_size < msg_size) {
		VPU_DEBG("Size allocated for Mbox message is not enough.\n");
		return VPU_STATUS_INSUFFICIENT_SPACE;
	} else {
		return VPU_STATUS_NOP;
	}
}

struct vpul_pu* find_pu_by_pu_location_struct_ptr(
	struct vpul_pu_location *current_pu_loc_ptr,
	const struct vpul_task	*inp_task_ptr){

	/* declarations */
	struct vpul_subchain *lcl_subchain_list_ptr;
	struct vpul_subchain *lcl_subchain_ptr;
	struct vpul_pu *lcl_pu_list_ptr;
	struct vpul_pu *outp_pu_ptr;


	/* start */

	/* get SC list of relevant vtx */
	lcl_subchain_list_ptr = fst_vtxidx_sc_ptr(inp_task_ptr, current_pu_loc_ptr->vtx_idx);
	/* get SC by it's index (pointer arithmetic) */
	lcl_subchain_ptr = lcl_subchain_list_ptr + current_pu_loc_ptr->sc_idx_in_vtx;
	/* get PU list for the SC */
	lcl_pu_list_ptr = fst_sc_pu_ptr(inp_task_ptr, lcl_subchain_ptr);
	/* Set Output: get specific PU in PU list  (pointer arithmetic) */
	outp_pu_ptr = lcl_pu_list_ptr + current_pu_loc_ptr->pu_idx_in_sc;

	return outp_pu_ptr;
}

/*
	get pointer to PU location by pu-location struct
	the function gets an input to the pu-location_struct in task,
	and returns pointer to the described PU in the TDS.
*/
struct vpul_pu* find_pu_by_pu_location_struct_idx
				(
				__u32	inp_pu_location_idx,
				const struct vpul_task	*inp_task_ptr)
{
	/* declarations */

	struct vpul_pu_location *current_pu_loc;
	struct vpul_pu *outp_pu_ptr;


	/* start */

	/* get pu location struct */
	current_pu_loc = updateble_pu_location_ptr(inp_task_ptr,inp_pu_location_idx);

	/* Set Output: get specific PU in PU list  (pointer arithmetic) */
	outp_pu_ptr = find_pu_by_pu_location_struct_ptr(current_pu_loc,inp_task_ptr);

	return outp_pu_ptr;
}


/* check that vtx vertex is valid. call function for all next vertexes */
static __s32 check_vertex(
	const struct vpul_vertex *vtxbase,
	const struct vpul_vertex *vtx,
	__u16 *vtxusd,
	__u32 gr_mdepth)
{
	__u32 vidx = VTXIDX(vtxbase, vtx);
	__u32 i;

	if ((vtx->n_out_edges == 0) && (vtx->vtype != VPUL_VERTEXT_END)) {
		VPU_ERRO("vertex index %d - no outputs and not end", vidx);
		return VPU_STATUS_ILLEGAL_TASK;
	}

	if (vtx->n_out_edges > VPUL_MAX_INOUT_EDGES) {
		VPU_ERRO("ver' out edges must be <= %d", VPUL_MAX_INOUT_EDGES);
		return VPU_STATUS_ILLEGAL_TASK;
	}

	if (vtx->vtype >= VPUL_VERTEXT_NUM) {
		VPU_ERRO("wrong vertex type for vtx idx %d", vidx);
		return VPU_STATUS_ILLEGAL_TASK;
	}

	if ((vtx->loop.type==VPU_LOOP_END)&&(vtx->loop.n_end_loop_edges >= vtx->n_out_edges)&&
		(vtx->n_out_edges != 0)) {
		VPU_ERRO("vtx idx %d: n_end_loop_edges > n_out_edges", vidx);
		return VPU_STATUS_ILLEGAL_TASK;
	}

	/* TODO: report error on vtxusd[vidx] != 0 */
	vtxusd[vidx] = 1;

	if (gr_mdepth == 0) {
		VPU_NOTE("can't check graph. vtx %d depth > max depth", vidx);
		return VPU_STATUS_NOP;
	}
	gr_mdepth--;

	/* check vertex edges */
	for (i = 0; i < vtx->n_out_edges; i++) {
		const struct vpul_vertex *nxtvtx = NxtVtxPtr(vtxbase, vtx, i);

		if (vtxusd[VTXIDX(vtxbase, nxtvtx)] == 0) {
			__s32 status = check_vertex(
				vtxbase,
				nxtvtx,
				vtxusd,
				gr_mdepth);

			if (status != VPU_STATUS_SUCCESS)
				return status;
		}
	}

	return VPU_STATUS_SUCCESS;
}

/* check graph test results */
static __s32 cgraph_result(__u32 num_of_ver, __u16 *vtxusd)
{
	__s32 ret = VPU_STATUS_SUCCESS;
	__u32 i;

	for (i = 0; i < num_of_ver; i++) {
		if (vtxusd[i] == 0) {
			ret = VPU_STATUS_ILLEGAL_TASK;
			break;
		}
	}
	return ret;
}

/* check first vertex is start, theres path to end vertex,
 * all vertexes are on graph. only one start, one end vertex,
 * edges, loop info is valid, etc..
 */
static __s32 check_task_graph(const struct vpul_task *task)
{
	__s32	ret;
	__u32	gr_mdepth = VPUL_MAX_GRAPH_DEPTH;
	struct vpul_vertex *vtx = fst_vtx_ptr(task);
	/* buffer for graph connection tests */
	__u16	vtx_usd[VPUL_MAX_VERTEX_ON_TASK];

	if (vtx->vtype != VPUL_VERTEXT_START) {
		VPU_ERRO("first vertex type must be start");
		return VPU_STATUS_ILLEGAL_TASK;
	}else if (task->t_num_of_vertices > VPUL_MAX_VERTEX_ON_TASK) {
		VPU_NOTE("can't perform graph check. too many vertices");
		ret = VPU_STATUS_SUCCESS;
	} else {
		memset(vtx_usd, 0, sizeof(vtx_usd));
		ret = check_vertex(vtx, vtx, vtx_usd, gr_mdepth);
		if (ret == VPU_STATUS_SUCCESS)
			ret =
			cgraph_result(task->t_num_of_vertices, vtx_usd);
	}

	return ret;
}

/* validate task data structure - connectivity, edges, loops */
static __s32 check_task_ds(const struct vpul_task *task)
{
	__s32 stat = VPU_STATUS_SUCCESS;

	if ((task->priority < VPUL_TASK_PRIORITY_MIN_VAL) ||
		(task->priority > VPUL_TASK_PRIORITY_MAX_VAL)) {
		VPU_ERRO("Illegal task priority - %d", task->priority);
		return VPU_STATUS_ILLEGAL_TASK;
	}

	/* check graph */
	stat = check_task_graph(task);

	return stat;
}

static void calc_num_of_edges(
	const struct vpul_task *task, __u8 *tot_edges)
{
	struct vpul_vertex *vtx = fst_vtx_ptr(task);
	__u32 i;

	*tot_edges = 0;
	for (i = 0; i < task->t_num_of_vertices; i++, vtx++) {
		(*tot_edges) += vtx->n_out_edges;
	}
}

static void set_task_loops(
	const struct vpul_task *task,
	const struct vpul_vertex *vtxbase,
	const struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp)
{
	const struct vpul_vertex *vtx = vtxbase;
	struct VPUI_TaskVertex *mb_vtx =
		(struct VPUI_TaskVertex *)(mb_cp + mb_tc->DataOfs.Vertex);
	struct VPUI_TaskLoop *mb_loops =
		(struct VPUI_TaskLoop *)(mb_cp + mb_tc->DataOfs.TaskLoops);
	struct VPUI_LoopParamsOfs *mb_lofs = &mb_loops->ParamsOffsets;
	struct VPUI_TaskEdge *mb_edges =
		(struct VPUI_TaskEdge *)(mb_cp + mb_tc->DataOfs.Edges);
	__u32 mb_loop_idx = 0;
	__u32 i;

	for (i = 0; i < task->t_num_of_vertices; i++, vtx++, mb_vtx++) {
		__u32 dst_vtx_id = 0;

		switch (vtx->loop.type) {
		case VPU_LOOP_END:
			mb_vtx->LoopEndIndP1 = vtx->loop.id+1;
			mb_vtx->LoopOpIndP1 = 0;
			mb_loops->LoopsNum = vtx->loop.iterations;
			mb_loops->DummyAlign = 0;
			mb_loops->ParamsTotals.ExpPerSet = 0;
			mb_loops->ParamsTotals.SetsNum = 0;
			mb_loops->ParamsTotals.FixInc = 0;
			mb_loops->ParamsTotals.PerFst = 0;
			mb_lofs->PerSet.FstInd = 0;
			mb_lofs->PerSet.FstVal = 0;
			mb_lofs->FixUpdate.FstInd = 0;
			mb_lofs->FixUpdate.FstVal = 0;
			mb_lofs->FstLoop.FstInd = 0;
			mb_lofs->FstLoop.FstVal = 0;
			dst_vtx_id = (mb_edges +
				mb_vtx->FstOutEdgeIndex)->DestVerInd;
			(mb_vtx + dst_vtx_id)->LoopOpIndP1 = mb_loop_idx;
			mb_loop_idx++;
			mb_loops++;
			break;
		case VPU_LOOP_START:
			mb_vtx->LoopEndIndP1 = 0;
			mb_vtx->LoopOpIndP1 = vtx->loop.id+1;
			break;
		default:
			/* For case VPU_LOOP_NONE */
			mb_vtx->LoopOpIndP1 = 0;
			mb_vtx->LoopEndIndP1 = 0;
			break;
		}
	}
}

/* fill Mbox command with vertices, edges and loops information */
static __s32 set_task_level_descriptors(
	const struct internal_context* intrnl_ctx,
	const struct vpul_task *task,
	const struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp)
{
	__u32 i, m;
	struct vpul_vertex *vtxbase = fst_vtx_ptr(task);
	struct vpul_vertex *vtx = vtxbase;
	struct VPUI_TaskVertex *mb_vtx =
		(struct VPUI_TaskVertex *)(mb_cp + mb_tc->DataOfs.Vertex);
	struct VPUI_TaskEdge *mb_edges =
		(struct VPUI_TaskEdge *)(mb_cp + mb_tc->DataOfs.Edges);
	struct VPUI_ProcDesc *mb_proc =
		(struct VPUI_ProcDesc *)(mb_cp + mb_tc->DataOfs.Procs);
	struct VPUI_Proc3DNNBase *mb_3dnn_1st_proc_base =
		(struct VPUI_Proc3DNNBase *)(mb_cp + mb_tc->DataOfs.Procs3DNNBase);
	struct VPUI_Proc3DNNBase *mb_3dnn_proc_base;


	__u32 mb_out_edges_idx = 0;
	__u32 vidx = 0;
	__u32 prev_vtx_params = 0;


	for (i = 0; i < task->t_num_of_vertices; i++, vtx++, mb_vtx++) {
		mb_vtx->Type = vtx->vtype;
		switch (vtx->vtype) {
		case VPUL_VERTEXT_PROC:
			mb_vtx->Index = vidx++;
			mb_vtx->FstParamOfs = prev_vtx_params;
			mb_vtx->TotalParams = /*TODO: possible lose of data */
				(__u8)mb_proc->Totals.All.ParamsValues;
			prev_vtx_params += mb_vtx->TotalParams;
			mb_proc++;
			break;
		case VPUL_VERTEXT_3DNN_PROC:
			mb_vtx->Index = vidx++;
			/* 3DNN processes are handled separately (below) */
			break;
		case VPUL_VERTEXT_END:
		case VPUL_VERTEXT_HOST_REP:
			mb_vtx->Index = vidx++;
			/*vals below may be updated in set_process_level_descriptors*/
			mb_vtx->FstParamOfs = 0;
			mb_vtx->TotalParams = 0;
			break;
		default:
			mb_vtx->Index = 0;
			mb_vtx->FstParamOfs = 0;
			mb_vtx->TotalParams = 0;
			break;
		}


		mb_vtx->InEdgesGroupsNum = 0;
		mb_vtx->FstInEdgesGrIndex = 0;

		/* output edges */
		mb_vtx->OutAllEdgesNum = vtx->n_out_edges;
		mb_vtx->FstOutEdgeIndex = mb_out_edges_idx;
		mb_out_edges_idx += vtx->n_out_edges;
		mb_vtx->EndLoopEdgesNum = (vtx->loop.type==VPU_LOOP_END )?
		    vtx->loop.n_end_loop_edges : 0;

		for (m = 0; m < vtx->n_out_edges; m++) {
		    struct vpul_edge *edge = &vtx->out_edges[m];

		    mb_edges->DestVerInd = edge->dst_vtx_idx;
		    mb_edges->DestGrIndP1 = 0;
		    mb_edges->IndInDestGr = 0;
		    mb_edges->DisableBitP1 = 0;
		    mb_edges++;
		}
	}

	/* separate handling for 3DNN processes : */
	vtx = vtxbase;
	mb_vtx = (struct VPUI_TaskVertex *)(mb_cp + mb_tc->DataOfs.Vertex);

	for (i = 0; i < task->t_num_of_vertices; i++, vtx++, mb_vtx++) {
		if (vtx->vtype == VPUL_VERTEXT_3DNN_PROC) {
			mb_vtx->FstParamOfs = prev_vtx_params;
			mb_3dnn_proc_base = &(mb_3dnn_1st_proc_base[vtx->proc3dnn.base_3dnn_ind]);
			mb_vtx->TotalParams = /*TODO: possible lose of data */
				(__u8)mb_3dnn_proc_base->Totals.All.ParamsValues;
			prev_vtx_params += mb_vtx->TotalParams;
		}
	}

	/* loop */
	set_task_loops(task, vtxbase, mb_tc, mb_cp);

	return VPU_STATUS_SUCCESS;
}

/* calculates number of invoke params according to invoke updatable PU's */
static __u32 calc_mb_create_invoke_param_size_from_task(
	const struct vpul_task *inp_task_ptr)
{
	__u32 sizeb = 0;
	__u32 updatable_pus_counter;
	struct vpul_pu_location *pu_loc_strct_ptr;
	struct vpul_pu *current_pu_ptr;
	__u32 current_pu_params_size;


	if (inp_task_ptr->t_num_of_pu_params_on_invoke>0)
	{
		pu_loc_strct_ptr = fst_updateble_pu_location_ptr(inp_task_ptr);
		for (updatable_pus_counter = 0;
			updatable_pus_counter<(inp_task_ptr->t_num_of_pu_params_on_invoke);
			updatable_pus_counter++
			)
		{
			current_pu_ptr = find_pu_by_pu_location_struct_idx(updatable_pus_counter,inp_task_ptr);
			current_pu_params_size = pu_get_num_params(current_pu_ptr);
			if (current_pu_params_size == 0)
				return 0;
			sizeb += current_pu_params_size;
		}
	}
	return sizeb;
}




/* calculates mailbox parameters size in bytes */
/* calculates mailbox parameters size in bytes */
static __u32 calc_mb_create_param_size_from_task(
	const struct vpul_task *task)
{
	/* TODO: consider if implement this function by calling
	 * set_task_create_command without writing to mb or by rounding up
	 * sizes - (it will be much faster..)
	 */
	__u32 sizeb = VPU_MAX_MB_CREATE_TASK_PARAM_SIZE;

	sizeb += task->t_num_of_vertices * sizeof(struct VPUI_ProcDesc);
	sizeb += task->t_num_of_subchains *
		(sizeof(struct VPUI_SubCh) + sizeof(struct VPUI_HwSubCh));
	sizeb += task->t_num_of_pus * sizeof(struct VPUI_BlUsage);
	sizeb += calc_mb_create_invoke_param_size_from_task(task) * sizeof(__u16);
	sizeb = (sizeb + 0x3FF) & ~0x3FF;
	return sizeb;
}

/* calculate create task Mbox message size */
static __u32 calc_mb_create_task_msg_size(const struct vpul_task *task)
{
	return  sizeof(struct VPUM_HostCmndHdr) +
		sizeof(struct VPUI_CmndHdr) +
		sizeof(struct VPUI_TaskCreate) +
		calc_mb_create_param_size_from_task(task);
}


/* pre-condition: all fields on mb_tot are valid with task sizes */
/* TODO: consider to change Mbox descriptor vectors order */
static __u32 mb_desc_vectors_offs(
	const struct VPUI_TaskTotals *mb_tot,
	struct VPUI_TaskCreateOfs *mb_ofs,
	__u32 start_offset)
{
	const struct VPUI_AllTotals *mb_at = &mb_tot->AllTotals;
	const struct VPUI_ProcAndTaskTotals *mb_pt = &mb_tot->ProcTotals;
	__u32 ofs = start_offset;

	mb_ofs->Blocks = ofs;
	ofs += m2size(sizeof(struct VPUI_BlUsage) * mb_at->HwSubCh.Blocks);
	mb_ofs->AllParamsValues = ofs;
	ofs += m2size(sizeof(__u16) * mb_at->ParamsValues);
		mb_ofs->HwConnect = ofs;
	ofs += m2size(sizeof(struct VPUI_HWConnect) *
		mb_at->HwSubCh.HwConnections);
	mb_ofs->InternalRamsInd = ofs;
	ofs += m2size(sizeof(__u8) * mb_at->HwSubCh.InternalRamsInd);
	mb_ofs->InternalRams = ofs;
	ofs += m2size(sizeof(struct VPUI_InternalRam) *
		mb_at->InternalRams);
	mb_ofs->HwReadResBl = ofs;
	ofs += m2size(sizeof(__u8) * mb_at->HwSubCh.HwReadResBl);
	mb_ofs->CpuOps = ofs;
	ofs += m2size(sizeof(struct VPUI_CpuOp) * mb_at->CpuOps);
	mb_ofs->CpuSubCh = ofs;
	ofs += m2size(sizeof(struct VPUI_CpuSubCh) * mb_at->CpuSubCh);
	/* Filters */
	mb_ofs->SepFiltCoeffSets = ofs;
	ofs += m2size(sizeof(struct VPUI_FiltCoeffsSets) *
		mb_tot->SepFiltCoeffSets);
	mb_ofs->GenFiltCoeffSets = ofs;
	ofs += m2size(sizeof(struct VPUI_FiltCoeffsSets) *
		mb_tot->GenFiltCoeffSets);
	mb_ofs->FiltStaticCoeffs = ofs;
	ofs += m2size(sizeof(__s16) * mb_tot->FiltStaticCoeffs);

	mb_ofs->HistParams = ofs;
	ofs += m2size(sizeof(struct VPUI_HistParams) * mb_tot->HistParams);
	mb_ofs->DisparityFixedThr = ofs;
	ofs += m2size(sizeof(struct VPUI_DisparityFixedThr) *
		mb_tot->DisparityFixedThr);

	mb_ofs->DepthDynThr = ofs;
	ofs += m2size(sizeof(struct VPUI_DepthDynThr) *
		mb_tot->DepthDynThr);

	mb_ofs->InPaintDynThr = ofs;
	ofs += m2size(sizeof(struct VPUI_InPaintDynThr) *
		mb_tot->InPaintDynThr);

	mb_ofs->OutputOffsets = ofs;
	ofs += m2size(sizeof(__u16) * mb_tot->OutputOffsets);
	mb_ofs->SubCh = ofs;
	ofs += m2size(sizeof(struct VPUI_SubCh) * mb_at->SubCh);
	mb_ofs->HwSubCh = ofs;
	ofs += m2size(sizeof(struct VPUI_HwSubCh) * mb_pt->HwSubCh);
	mb_ofs->InOutTypes = ofs;
	ofs += m2size(sizeof(struct VPUI_InOutType) * mb_pt->InOutTypes);
	mb_ofs->PtLists = ofs;     /* Offset to vector of VPUI_PtList.   */
	ofs += m2size(sizeof(struct VPUI_PtList) * mb_pt->PtLists);
	mb_ofs->PtRoiSizes = ofs;  /* Offset to vector of VPUI_Sizes.    */
	ofs += m2size(sizeof(struct VPUI_Sizes) * mb_pt->PtRoiSizes);
	mb_ofs->RoiFromPt = ofs;   /* Offset to vector of VPUI_RoiFromPt.*/
	ofs += m2size(sizeof(struct VPUI_RoiFromPt) * mb_pt->RoiFromPt);
	mb_ofs->PtMapIndices = ofs;/* Offset to vector of uint8          */
	ofs += m2size(sizeof(__u8) * mb_pt->PtMapIndices);
	mb_ofs->FixPoints = ofs;   /* Offset to vector of VPUI_Point.    */
	ofs += m2size(sizeof(struct VPUI_Point) * mb_pt->FixPoints);
	mb_ofs->Rois = ofs;
	ofs += m2size(sizeof(struct VPUI_Roi) * mb_pt->Rois);
	mb_ofs->FixMapRois = ofs;
	ofs += m2size(sizeof(struct VPUI_FixMapRoi) * mb_pt->FixMapRois);
	mb_ofs->SizesOp = ofs;
	ofs += m2size(sizeof(struct VPUI_SizesOp) * mb_pt->SizesOp);
	mb_ofs->Tiles = ofs;
	ofs += m2size(sizeof(struct VPUI_Tile) * mb_tot->Tiles);
	mb_ofs->FixOps = ofs;
	ofs += m2size(sizeof(struct VPUI_FixOp) * mb_tot->FixOps);
	mb_ofs->CropOps = ofs;
	ofs += m2size(sizeof(struct VPUI_CropOp) * mb_tot->CropOps);
	mb_ofs->ScaleOps = ofs;
	ofs += m2size(sizeof(struct VPUI_ScaleOp) * mb_tot->ScaleOps);
	mb_ofs->Hw3DNNSlGrTypeSubCh = ofs;
	ofs += m2size(sizeof(struct VPUI_Hw3DNNSlGrTypeSubCh) *
		mb_tot->Proc3DNNTotals.Hw3DNNSlGrTypeSubCh);
	mb_ofs->Hw3DNNSubCh = ofs;
	ofs += m2size(sizeof(struct VPUI_Hw3DNNSubCh) *
		mb_tot->Proc3DNNTotals.Hw3DNNSubCh);
	mb_ofs->IoXY3DNNSizes = ofs;
	ofs += m2size(sizeof(struct VPUI_3DNNIoXYSizes) *
		mb_tot->Proc3DNNTotals.IoXY3DNNSizes);
	mb_ofs->InOut3DNN = ofs;
	ofs += m2size(sizeof(struct VPUI_InOut3DNN) *
		mb_tot->Proc3DNNTotals.InOut3DNN);
	mb_ofs->CropInd = ofs;
	ofs += m2size(sizeof(__u8) * mb_tot->Proc3DNNTotals.CropInd);
	mb_ofs->ScaleInd = ofs;
	ofs += m2size(sizeof(__u8) * mb_tot->Proc3DNNTotals.ScaleInd);
	mb_ofs->Sizes3DOp = ofs;
	ofs += m2size(sizeof(struct VPUI_3DSizesOp) *
		mb_tot->Proc3DNNTotals.Sizes3DOp);
	mb_ofs->Layers3DNN = ofs;
	ofs += m2size(sizeof(struct VPUI_3DNNLayer) *
		mb_tot->Proc3DNNTotals.Layers3DNN);
	mb_ofs->Procs3DNN = ofs;
	ofs += m2size(sizeof(struct VPUI_Proc3DNNDesc) * mb_tot->Procs3DNN);
	mb_ofs->CnnCropDesc = ofs;
	ofs += m2size(sizeof(struct VPUI_CnnCropDesc) * mb_tot->CnnCropDesc);
	mb_ofs->ReluDesc = ofs;
	ofs += m2size(sizeof(struct VPUI_CnnReluDesc) * mb_tot->ReluDesc);
	mb_ofs->CNNScalers = ofs;
	ofs += m2size(sizeof(__u16) * mb_tot->CNNScalers);
	mb_ofs->PreMemVecDesc = ofs;
	ofs += m2size(sizeof(struct VPUI_MemVecDesc) *
		mb_tot->ProcTotals.PreSetsMemVecs);
	mb_ofs->PostMemVecDesc = ofs;
	ofs += m2size(sizeof(struct VPUI_MemVecDesc) *
		mb_tot->ProcTotals.PostSetsMemVecs);
	mb_ofs->MapDesc = ofs;
	ofs += m2size(sizeof(struct VPUI_MapDesc) * mb_tot->MapDesc);
	mb_ofs->SlotOfs = ofs;
	ofs += m2size(sizeof(struct VPUI_32bMem) * mb_tot->ExtSlotOfs);
	mb_ofs->SlotMem = ofs;
	ofs += m2size(sizeof(struct VPUI_SlotMem) * mb_tot->SlotMem);
	mb_ofs->EachInvokeExtMemAdrInd = ofs;
	ofs += m2size(sizeof(__u8) * mb_tot->EachInvokeExtMemAdr);
	mb_ofs->Vertex = ofs;
	ofs += m2size(sizeof(struct VPUI_TaskVertex) * mb_tot->Vertex);
	mb_ofs->Edges = ofs;
	ofs += m2size(sizeof(struct VPUI_TaskEdge) * mb_tot->Edges);
	mb_ofs->InEdgesGroups = ofs;
	ofs += m2size(sizeof(__u8) * mb_tot->InEdgesGroups);
	mb_ofs->TaskLoops = ofs;
	ofs += m2size(sizeof(struct VPUI_TaskLoop) * mb_tot->TaskLoops);
	mb_ofs->AllParamsIndices = ofs; /* offset to indices -!offsets!- as in VPUI_TaskParamsIndicesOfs */
	ofs += m2size(sizeof(__u16) * mb_at->ParamsIndices); /* add size , as kept on ParamsIndices */

	return ofs;
}

/* set and return pointer to command header */
static struct VPUI_Cmnd *cmd_header(void *mailbox_command,
	__u32 mb_cmd_type, __u32 task_id)
{
	struct VPUI_Cmnd *mb_cmd = (struct VPUI_Cmnd *)((__u8 *)mailbox_command
		+ sizeof(struct VPUM_HostCmndHdr));

	mb_cmd->Hdr.CmndType = mb_cmd_type;
	mb_cmd->Hdr.TaskId = task_id;
	return mb_cmd;
}

static __s32 prepare_short_mb_command(
	const struct vpul_task *task,
	size_t *mailbox_command_size,
	void *mailbox_command,
	enum VPUI_CmndType cmd_type)
{
	__s32 stat;
	__u32 mb_msg_size;

	if (task == NULL)
		return VPU_STATUS_BAD_PARAMS;

	/* calculate Mbox message size */
	mb_msg_size = task->task_ro.mbox_msg_sizes.short_msg;
	stat = check_size(mb_msg_size, mailbox_command_size, mailbox_command);
	if (stat == VPU_STATUS_NOP) {
		stat = VPU_STATUS_SUCCESS;
		cmd_header(mailbox_command, cmd_type, task->id);
	}
	return stat;
};

/* set memory information for core init command */
static void set_mem_info(
	const struct vpul_memory_info *mem_inf,
	struct VPUI_MemInfo *mb_mem)
{
	mb_mem->MaxTasks = mem_inf->max_tasks;
	mb_mem->QuProcVertices = mem_inf->max_proc_vertices;
	mb_mem->QuControlVertices = mem_inf->max_control_vertices;
	mb_mem->MinimalAllocSize = mem_inf->min_alloc_size;
	mb_mem->MaximalValidUnusedSize = mem_inf->max_valid_unused_size;
}

/* set time information for core init command */
static void set_time_info(
	const struct vpul_time_info *time_inf,
	struct VPUI_TimeInfo *mb_time)
{
	mb_time->IspClkMhz = time_inf->isp_clock_mhz;
	mb_time->HwTimeOutMilliSec = time_inf->hw_timeout_msec;
	mb_time->ProcessTimeOutMilliSec = time_inf->process_timeout_msec;
	mb_time->TaskTimeOutMilliSec = time_inf->task_timeout_msec;
}

static void set_host(
	const struct vpul_mbox_host_data *ds_host,
	struct VPUM_HostMboxData *mb_host)
{
	mb_host->Size16b = ds_host->size_in_u16;
	mb_host->SendResponseOnErrorOnly = ds_host->send_response_on_error_only;
}

static void set_core(
	const struct vpul_mbox_core_data *ds_core,
	struct VPUM_CoreMboxData *mb_core)
{
	mb_core->MsgNum = ds_core->n_messages;
	mb_core->IntrType = ds_core->interrupt_type;
}

/* set mbox information for core init command */
static void set_mbox_info(
	const struct vpul_mbox_init_info *inf,
	struct VPUM_MboxInitCmndInfo *mb)
{
	set_host(&inf->host_normal,
		&mb->HostMboxData[VPUM_HOST_MBOX_NORMAL_CMND]);
	set_host(&inf->host_lowpr,
		&mb->HostMboxData[VPUM_HOST_MBOX_LOW_PR_CMND]);

	set_core(&inf->core_reports,
		&mb->CoreMboxData[VPUM_CORE_MBOX_REPORTS]);
	set_core(&inf->core_create_sizes,
		&mb->CoreMboxData[VPUM_CORE_MBOX_CREATE_SIZES]);
	set_core(&inf->core_syserr,
		&mb->CoreMboxData[VPUM_CORE_MBOX_SYS_ERRORS]);
	set_core(&inf->core_normal,
		&mb->CoreMboxData[VPUM_CORE_MBOX_NORMAL_CMND_RESPONSE]);
	set_core(&inf->core_lowpr,
		&mb->CoreMboxData[VPUM_CORE_MBOX_LOW_PR_CMND_RESPONSE]);
	set_core(&inf->core_debug,
		&mb->CoreMboxData[VPUM_CORE_MBOX_DEBUG_INFO]);

	mb->VerifyIncCmndInd = inf->verify_inc_cmd_id;
	mb->UpdateMboxSizes = inf->update_sizes;
	mb->ReportDataSize = inf->report_data_size;
}

/* this function returns a pointer to vertex referencing a 3DNN process base specified at input
 * returns NULL if no such vertex found
 * if more than 1 vertex found, returns the one with largest number of subchains
 * inputs : task - pointer to TDS
 *          proc_base_3dnn_idx - index of specified 3DNN process base
 */
struct vpul_vertex *vertex_referencing_this_3dnn_proc_base(const struct vpul_task *task,
								__u32 proc_base_3dnn_idx)
{
	struct vpul_vertex *vtx = fst_vtx_ptr(task);
	struct vpul_vertex *found_vtx = NULL;
	__u32 i;
	__u32 max_subch = 0;

	for (i = 0; i < task->t_num_of_vertices; i++, vtx++) {
		if (vtx->vtype == VPUL_VERTEXT_3DNN_PROC) {
			if (vtx->proc3dnn.base_3dnn_ind == proc_base_3dnn_idx) {
				/* found a suitable vertex, check number of subchains */
				if (vtx->num_of_subchains >= max_subch) {
					max_subch = vtx->num_of_subchains;
					found_vtx = vtx;
				}
			}
		}
	}
	return found_vtx;
}

static __s32 set_task_invoke_mem_indc(
	const struct vpul_task *task,
	const struct vpu_mark_for_invoke_ext_mem_vec_ds *invoke_mem_mark,
	const struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp
		)
{
	__u32 i;
	__u8 *extern_idx_for_invoke = NULL;

	BUG_ON(task            == NULL);
	BUG_ON(invoke_mem_mark == NULL);
	BUG_ON(mb_tc           == NULL);
	BUG_ON(mb_cp           == NULL);
	BUG_ON(mb_tc->Totals.EachInvokeExtMemAdr !=
		invoke_mem_mark->num_of_buffers);

	extern_idx_for_invoke = (__u8 *)(mb_cp +
		mb_tc->DataOfs.EachInvokeExtMemAdrInd);

	for (i = 0; i < mb_tc->Totals.EachInvokeExtMemAdr; i++) {
		*(extern_idx_for_invoke) =
		 (__u8)invoke_mem_mark->external_mem_index[i];
		extern_idx_for_invoke++;
	}

	return VPU_STATUS_SUCCESS;

}

/* return the number of mprb groups needed to describe all mprb needed for these ports*/
static __u32 get_mprb_number_of_groups(
	__u32 port_bit_map,
	const struct mprb_entry * mprb_entry )
{
	__u32 count = 0;
	__u32 i = 0;
	__u32 bit_mask = 1;
	__u32 current_mprb = 0;
	__u32 last_mprb = 0;

	for(;i < 8;i++){
		if((port_bit_map&bit_mask) != 0){
			current_mprb =  mprb_entry->preloaded_mprb[i] + 1;
			/* First MPRB found is means new group */
			if(last_mprb==0){
				BUG_ON(count!=0);
				count=1;
			}
			/* If not in sequence to last MPRB open new group */
			else if(last_mprb!= current_mprb-1 ){
				count++;
			}
			last_mprb = current_mprb;
		}
		bit_mask*=2;
	}
	return count;
}
static __s32 set_task_mem_desc(
	struct internal_context * internal_context,
	const struct vpul_task *task,
	const struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp
	)
{
	__u32 i;
	__u32 image_desc_idx  = 0;
	__u32 intram_idx=0;
	__u8 *intram_idxs = (__u8 *)(mb_cp + mb_tc->DataOfs.InternalRamsInd);
	__u16 *mb_params  = (__u16 *)(mb_cp + mb_tc->DataOfs.AllParamsValues);
	__u16* imgdesc_prm  = (__u16 *)(mb_params + mb_tc->ParamsParts.ValuesOfs.ImageDesc);
	struct VPUI_MapDesc *map_desc =
		(struct VPUI_MapDesc *)(mb_cp + mb_tc->DataOfs.MapDesc);
	struct VPUI_InternalRam *int_rams =
		(struct VPUI_InternalRam *)(mb_cp + mb_tc->DataOfs.InternalRams);

	__s32 status = VPU_STATUS_SUCCESS;

	__u16 FstMprbGrOfs = 0; /* offset accumulator */
	__u8  MprbsGrNum;


	/* Maps descriptions vector, Memory descriptors vectors */
	for (i = 0; i < task->n_memmap_desc; i++) {
		const struct vpul_memory_map_desc *memd = &task->memmap_desc[i];

		map_desc->Ind = memd->index;
		map_desc->DummyAlign = 0;
		switch (memd->mtype) {
		case VPUL_MEM_EXTERNAL:
			map_desc->Type = VPUI_MEM_EXTERNAL;
			break;

		case VPUL_MEM_PRELOAD_PU:
		case VPUL_MEM_INTERNAL:
			{
				const struct vpul_image_size_desc * image_sizes
					= &(memd->image_sizes);

				map_desc->Type = VPUI_MEM_INTERNAL;
				int_rams[intram_idx].FstMprbGrOfs = 0;
				int_rams[intram_idx].MprbsGrNum = 0;
				int_rams[intram_idx].VirtualUnitsNum = 0;

				if(memd->mtype==VPUL_MEM_PRELOAD_PU){
					const struct mprb_entry * entry =
						get_next_mprb_match( internal_context,
						memd->pu_index.proc,
						memd->pu_index.sc,
						memd->pu_index.pu );

					if(entry==NULL){
						return VPU_STATUS_PRELOAD_MISMATCH;
					}
					MprbsGrNum = get_mprb_number_of_groups(memd->pu_index.ports_bit_map, entry);
				}
				else{
					MprbsGrNum   = task->internal_rams[memd->index].n_mprb_groups;
				}
				int_rams[intram_idx].FstMprbGrOfs = FstMprbGrOfs;
				int_rams[intram_idx].MprbsGrNum	= MprbsGrNum;
				int_rams[intram_idx].VirtualUnitsNum
					= ((image_sizes->width*image_sizes->height
					*image_sizes->pixel_bytes)+VPUI_IN_RAM_VIRTUAL_UNIT_SIZE-1)
					/VPUI_IN_RAM_VIRTUAL_UNIT_SIZE;
				*intram_idxs = map_desc->Ind = intram_idx;

				FstMprbGrOfs+=MprbsGrNum;
				intram_idx++;
				intram_idxs++;
				break;

			}

		case VPUL_MEM_COEFF_VEC:
			/* TODO:*/
			break;

		default:
			/* TODO: add error */
			break;

		}
		if (image_desc_idx < mb_tc->Totals.ProcTotals.InOutTypes) {
			*imgdesc_prm++ = memd->image_sizes.width;
			*imgdesc_prm++ = memd->image_sizes.height;
			*imgdesc_prm++ = memd->image_sizes.line_offset;
			*imgdesc_prm++ = memd->image_sizes.pixel_bytes;
		}
		map_desc->SizesInd = image_desc_idx++;
		map_desc++;
	}

	PAD_U8_PTR_TO_ALIGN(intram_idxs);
	BUG_ON(internal_context->n_internal_mem != intram_idx);

	return status;
}

static __s32 set_proc_inouts(
	struct internal_context* p_intrnl_cntxt,
	const struct vpul_process_inout *pio,
	const struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp,
	struct mb_ptrs *mbp
	)
{
	__u32 i;
	/* TODO: handle dynamic ROI */
	// loop for static
	for (i=0;i<pio->n_fixed_map_roi; i++)
	{
		mbp->fix_map_roi->MapInd = pio->fixed_map_roi[i].memmap_idx;
		if (pio->fixed_map_roi[i].use_roi == 0) {
			mbp->fix_map_roi->ROIIndP1 = 0;
			mbp->fix_map_roi->GroupInd = 0;
		}
		else
		{
			struct VPUI_Roi * roi_mb_ptr = (mbp->roi + p_intrnl_cntxt->tds_itrtr_stat.next_roi_index);


			const struct	vpul_roi* current_fixed_roi = &(pio->fixed_map_roi[i].roi);

			p_intrnl_cntxt->tds_itrtr_stat.next_roi_index++;

			mbp->fix_map_roi->ROIIndP1 = p_intrnl_cntxt->tds_itrtr_stat.next_roi_index;
			roi_mb_ptr->FstCol       = current_fixed_roi->first_col;
			roi_mb_ptr->FstLine      = current_fixed_roi->first_line;
			roi_mb_ptr->Sizes.Height = current_fixed_roi->height;
			roi_mb_ptr->Sizes.Width  = current_fixed_roi->width;

			mbp->fix_map_roi->GroupInd = 0; /* dont care: Rois are not divided to few groups */

		}
		mbp->fix_map_roi++;

	}
	/* in-out types */
	for (i = 0; i < pio->n_inout_types; i++) {

		mbp->inout_types->IsDyn = pio->inout_types[i].is_dynamic;
		if (pio->inout_types[i].is_dynamic) {
			/* TODO: handle dynamic ROI */
			mbp->inout_types->NumInpSetsEachRoi =
				pio->inout_types[i].n_insets_per_dynamic_roi;
		}
		else {
			mbp->inout_types->NumInpSetsEachRoi = 0;
		}

		if (pio->inout_types[i].is_roi_derived)
			mbp->inout_types->RoiFromPtIndP1 = i + 1;
		else
			mbp->inout_types->RoiFromPtIndP1 = 0;

		mbp->inout_types->TileIndP1 = 0;/*TODO: */
		mbp->inout_types->IsUpdateByLoop = 0;/*TODO: */
		mbp->inout_types->DummyAlign = 0;
		mbp->inout_types++;
	}

/*	mbp->fixed_map_idx += pio->n_fixed_map_roi; */

	mbp->inout_types_idx += pio->n_inout_types;

	/* Operation sizes */
	for (i = 0; i < pio->n_sizes_op; i++) {
		if(pio->sizes[i].type < VPUL_SIZEOP_NUM){
			mbp->size_op->SizesOpBits =
			(sizeop_type_lut[pio->sizes[i].type] << SIZES_OP_TYPE_LSB) |
			(pio->sizes[i].op_ind << SIZES_OP_IND_LSB);
			mbp->size_op->SrcInd = pio->sizes[i].src_idx;
			mbp->size_op++;
		}
		else{
			return VPU_STATUS_BAD_PARAMS;
		}
	}

	return VPU_STATUS_SUCCESS;
}

static __s32 set_proc_3dnn_inouts(const struct vpul_task *task,
				const struct vpul_3dnn_process_base *proc_3dnn_base,
				const struct VPUI_TaskCreate *mb_tc,
				__u8 *mb_cp,
				struct mb_ptrs *mbp)
{
	__u32 i, j;
	__u32 op_index;
	const struct vpul_3dnn_size *p_3dnn_size;
	const struct vpul_3dnn_layer *layer_3dnn;
	const struct vpul_inout_3dnn *p_inout_3dnn;
	const struct vpul_memory_map_desc *memmap_descr;
	const struct vpul_3dnn_process_inout *proc_3dnn_inout = &proc_3dnn_base->io;
	struct VPUI_3DSizesOp *mb_3dnn_size_op_vec =
				(struct VPUI_3DSizesOp *)(mb_cp + mb_tc->DataOfs.Sizes3DOp);
	struct VPUI_InOut3DNN *mb_inout_3dnn_vec =
		(struct VPUI_InOut3DNN *)(mb_cp + mb_tc->DataOfs.InOut3DNN);

	/* INOUT sizes */
	mb_inout_3dnn_vec += mbp->proc_3dnn_base_desc->Fst.InOut3DNN;
	for (i = 0; i < proc_3dnn_base->number_of_layers; i++) {
		layer_3dnn = &proc_3dnn_base->layers[i];
		for (j = 0; j < proc_3dnn_inout->n_3dnn_inouts; j++, mb_inout_3dnn_vec++) {
			p_inout_3dnn = &layer_3dnn->inout_3dnn[j];
			memmap_descr = &task->memmap_desc[p_inout_3dnn->mem_descr_index];
			mb_inout_3dnn_vec->BaseAdrInd = memmap_descr->index;
			mb_inout_3dnn_vec->InOut3DNNType = p_inout_3dnn->inout_3dnn_type;
			if (mb_inout_3dnn_vec->InOut3DNNType == VPUI_IO_3DNN_COEFFS)
				mb_inout_3dnn_vec->BytesNum =
				p_inout_3dnn->bytes_per_pixel_or_dma_bytes_width.dma_bytes_width;
			else
				mb_inout_3dnn_vec->BytesNum =
				p_inout_3dnn->bytes_per_pixel_or_dma_bytes_width.bytes_per_pixel;
			mb_inout_3dnn_vec->DummyAlign = 0;
		}
	}

	/* Operation sizes */
	mb_3dnn_size_op_vec += mbp->proc_3dnn_base_desc->Fst.Sizes3DOp;
	for (i = 0; i < proc_3dnn_inout->n_sizes_op; i++) {
		p_3dnn_size = &proc_3dnn_inout->sizes_3dnn[i];
		op_index = p_3dnn_size->op_ind;
		if (p_3dnn_size->type == VPUL_3DXY_SIZEOP_INOUT)
			op_index = p_3dnn_size->inout_3dnn_type;
		else if (p_3dnn_size->type >= VPUL_3DXY_SIZEOP_NUM)
			/* Not supported sizeop type */
			return VPU_STATUS_BAD_PARAMS;

		mb_3dnn_size_op_vec->Sizes3DOpBits =
				(sizeop_3dxy_type_lut[p_3dnn_size->type] << SIZES_3D_OP_TYPE_LSB) |
				(op_index << SIZES_3D_OP_IND_LSB);
		mb_3dnn_size_op_vec->SrcInd = p_3dnn_size->src_idx;
		mb_3dnn_size_op_vec++;
	}
	return VPU_STATUS_SUCCESS;
}

/* fill Mbox command with one HW subchain information */
static void set_hw_subchain_descriptor(
	struct internal_context * internal_context,
	const struct vpul_task *task,
	const struct vpul_subchain *sc,
	struct mb_ptrs *mbp,
	struct mbsc_cnts *pmbsc
	)
{
	struct VPUI_HwSubCh *hw_subch = mbp->hw_subch_desc;
	const struct vpul_pu *pu = fst_sc_pu_ptr(task, sc);
	__u32 i;

	hw_subch->SubChTotals.Blocks = sc->num_of_pus;
	hw_subch->SubChTotals.HwConnections = 0;
	hw_subch->SubChTotals.HwMprbsGr = 0;
	/* TODO: check connection number is valid according to block type */
	for (i = 0; i < sc->num_of_pus; i++, pu++) {
		hw_subch->SubChTotals.HwConnections += pu->n_in_connect;
		hw_subch->SubChTotals.HwMprbsGr += pu_get_num_mprb_groups(pu);
	}

	hw_subch->SubChFstIndOfs.Bl.Block = pmbsc->fst_block;
	pmbsc->fst_block += hw_subch->SubChTotals.Blocks;
	hw_subch->SubChFstIndOfs.Bl.Params = pmbsc->fst_bl_prm;
	pmbsc->fst_bl_prm += pmbsc->n_bl_params;

	/* Update Blocks Read Results Parameters */
	hw_subch->SubChFstIndOfs.Bl.HwReadResBl = pmbsc->fst_hw_read_res;
	hw_subch->SubChFstIndOfs.Bl.HwConnections = pmbsc->fst_connect;
	pmbsc->fst_hw_read_res += pmbsc->n_hw_read_params;


	hw_subch->SubChTotals.IntRamReadResBits = pmbsc->n_hw_read_params << HW_SUB_CH_READ_RES_BL_LSB |
		(internal_context->n_internal_mem-pmbsc->fst_intramidx) << HW_SUB_CH_INT_RAM_IND_LSB;


	hw_subch->SubChFstIndOfs.Other.DummyAlign = 0;
	pmbsc->fst_connect += hw_subch->SubChTotals.HwConnections;
	hw_subch->SubChFstIndOfs.Other.HwMprbsGr = pmbsc->fst_hwmprb;
	pmbsc->fst_hwmprb += hw_subch->SubChTotals.HwMprbsGr;
	hw_subch->SubChFstIndOfs.Other.IntRamInd = pmbsc->fst_intramidx;
	pmbsc->fst_intramidx = internal_context->n_internal_mem;

	mbp->hw_subch_desc++;
	mbp->subch_desc->Type = VPUI_SUB_CH_HW;
	mbp->subch_desc->HwIndex = pmbsc->hwsc_idx++;
}

/* fill Mbox command with one instance of VPUI_Hw3DNNSlGrTypeSubCh */
static void set_3dnn_hw_slgr_type_subch_descr(struct internal_context * internal_context,
						const struct vpul_task *task,
						const struct vpul_vertex *vtx,
						const struct vpul_subchain *sc,
						struct mb_ptrs *mbp,
						struct mbsc_cnts *pmbsc,
						__u32 hw_3dnn_slgr_type_subch_idx)
{
	struct VPUI_Hw3DNNSlGrTypeSubCh *hw_3dnn_slgr_type_subch =
						mbp->hw_3dnn_slgr_type_subch_desc;
	struct VPUI_Hw3DNNSubCh *hw_3dnn_subch = mbp->hw_3dnn_subch_desc;
	__u32 slgr_bitmap = sc->sl_group_type_ident.slice_group_bitmap;
	const struct vpul_pu *pu = fst_sc_pu_ptr(task, sc);
	__u32 i;

	hw_3dnn_slgr_type_subch->Totals.Blocks = sc->num_of_pus;
	hw_3dnn_slgr_type_subch->Totals.HwConnections = 0;
	hw_3dnn_slgr_type_subch->Totals.HwMprbsGr = 0;
	/* TODO: check connection number is valid according to block type */
	for (i = 0; i < sc->num_of_pus; i++, pu++) {
		hw_3dnn_slgr_type_subch->Totals.HwConnections += pu->n_in_connect;
		hw_3dnn_slgr_type_subch->Totals.HwMprbsGr += pu_get_num_mprb_groups(pu);
	}
	hw_3dnn_slgr_type_subch->FstBlInd.HwConnections = pmbsc->fst_connect;
	hw_3dnn_slgr_type_subch->FstBlInd.Block = pmbsc->fst_block;
	pmbsc->fst_block += hw_3dnn_slgr_type_subch->Totals.Blocks;
	hw_3dnn_slgr_type_subch->FstBlInd.Params = pmbsc->fst_bl_prm;
	pmbsc->fst_bl_prm += pmbsc->n_bl_params;

	/* Update Blocks Read Results Parameters */
	hw_3dnn_slgr_type_subch->FstBlInd.HwReadResBl = pmbsc->fst_hw_read_res;
	pmbsc->fst_hw_read_res += pmbsc->n_hw_read_params;

	hw_3dnn_slgr_type_subch->Totals.IntRamReadResBits = pmbsc->n_hw_read_params << HW_SUB_CH_READ_RES_BL_LSB |
		(internal_context->n_internal_mem-pmbsc->fst_intramidx) << HW_SUB_CH_INT_RAM_IND_LSB;

	mbp->hw_3dnn_slgr_type_subch_desc++;

	if (slgr_bitmap & SUBCH_SLICE_GROUP_FIRST)
		hw_3dnn_subch->SlGrInd[VPUI_3D_SLGR_FIRST] = hw_3dnn_slgr_type_subch_idx;
	if (slgr_bitmap & SUBCH_SLICE_GROUP_MIDDLE)
		hw_3dnn_subch->SlGrInd[VPUI_3D_SLGR_MIDDLE] = hw_3dnn_slgr_type_subch_idx;
	if (slgr_bitmap & SUBCH_SLICE_GROUP_LAST)
		hw_3dnn_subch->SlGrInd[VPUI_3D_SLGR_LAST] = hw_3dnn_slgr_type_subch_idx;
	if (slgr_bitmap & SUBCH_SLICE_GROUP_SINGLE)
		hw_3dnn_subch->SlGrInd[VPUI_3D_SLGR_SINGLE] = hw_3dnn_slgr_type_subch_idx;

	pmbsc->fst_connect += hw_3dnn_slgr_type_subch->Totals.HwConnections;
	pmbsc->fst_hwmprb += hw_3dnn_slgr_type_subch->Totals.HwMprbsGr;
	pmbsc->fst_intramidx = internal_context->n_internal_mem;
}

/* fill Mbox command with one instance of VPUI_Hw3DNNSubCh, and one instance of VPUI_SubCh
 * (for 3DNN subchain)
 */
static void set_3dnn_hw_subch_and_vpui_subch_descr(struct mb_ptrs *mbp,
						struct mbsc_cnts *pmbsc)
{
	struct VPUI_Hw3DNNSubCh *hw_3dnn_subch_descr = mbp->hw_3dnn_subch_desc;
	struct VPUI_SubCh *subch_descr = mbp->subch_desc;

	/* filling VPUI_SubCh */
	subch_descr->DisableBitIndP1 = 0;
	subch_descr->CpuIndex = 0;
	subch_descr->Type = VPUI_SUB_CH_HW;
	subch_descr->HwIndex = pmbsc->hwsc_idx++;
	mbp->subch_desc++;

	hw_3dnn_subch_descr->OtherOfs.DummyAlign = 0;
	hw_3dnn_subch_descr->OtherOfs.HwMprbsGr = pmbsc->fst_hwmprb;
	hw_3dnn_subch_descr->OtherOfs.IntRamInd = pmbsc->fst_intramidx;

	/* initialize all entries in SlGrInd - to avoid leaving "gaps" (detected as errors by fill
	 * test), in case some of the entries are left to uninitialized value = 0xbb because they
	 * are unused / not needed
	 */
	hw_3dnn_subch_descr->SlGrInd[VPUI_3D_SLGR_FIRST] = 0xFF;
	hw_3dnn_subch_descr->SlGrInd[VPUI_3D_SLGR_MIDDLE] = 0xFF;
	hw_3dnn_subch_descr->SlGrInd[VPUI_3D_SLGR_LAST] = 0xFF;
	hw_3dnn_subch_descr->SlGrInd[VPUI_3D_SLGR_SINGLE] = 0xFF;
}

/* fill Mbox command with one CPU operation information */
static void set_cpu_subchain_descriptor
(
	const struct vpul_task *task,
	const struct vpul_subchain *sc,
	struct mb_ptrs *mbp,
	struct mbsc_cnts *pmbsc,
	__u32  *cpu_subch_index
)
{
	mbp->subch_desc->Type = VPUI_SUB_CH_CPU_HIGH_PR;
	mbp->subch_desc->CpuIndex = *cpu_subch_index;
}

static void set_hw_sc_connections(
	const struct vpul_task *task,
	const struct vpul_subchain *sc,
	struct mb_ptrs *mbp
		)
{
	__u32 scs_pu_idx;
	const struct vpul_pu *pu = fst_sc_pu_ptr(task, sc);

	for (scs_pu_idx = 0; scs_pu_idx < sc->num_of_pus; scs_pu_idx++, pu++) {

		if (pu->n_in_connect != 0) {
			__u32 i = 0;

			for (i = 0; i < pu->n_in_connect; i++) {
				if (pu->in_connect[i].pu_idx == NO_PU_CONNECTED)
					mbp->connections_desc->SrcBlIndP1 = 0;
				else
					mbp->connections_desc->SrcBlIndP1 =
							pu->in_connect[i].pu_idx + 1;
				mbp->connections_desc->OutPortInd =
					pu->in_connect[i].s_pu_out_idx;
				mbp->connections_desc++;
			}
		}
	}
}

/* set process sub-chains */
static __s32 set_proc_subchains(struct internal_context *internal_context,
				const struct vpul_task *task,
				struct mb_ptrs *mbp)
{
	struct vpul_vertex *vtx = vtx_ptr(task, internal_context->tds_itrtr_stat.current_vtx_id);
	struct vpul_subchain *sc = fst_vtx_sc_ptr(task, vtx);
	struct mbsc_cnts mbsc = {0};
	__s32 stat;
	__u32 i;
	__u32 current_mprb_grp_offset_in_subchain;
	__u32 cpu_subch_index = 0;
	struct vpul_pu_location *init_pu_location_list = fst_updateble_pu_location_ptr(task);
	struct vpul_pu_location **pu_location_list_ptr = &(init_pu_location_list);
	__u16*  init_mb_invoke_prm     = (__u16 *)mbp->invoke_prm;
	__u16** init_mb_invoke_prm_ptr = &init_mb_invoke_prm; /* need pointer as an inout argument */
	__u16  accumulated_n_of_invoke_params = 0;
	__u16* accumulated_n_of_invoke_params_ptr = &accumulated_n_of_invoke_params;
	__u16 current_offset_from_param_start =
		mbp->block_prm - (mbp->first_proc_prm - VPUI_TASKPAR_ALL);
	__u32 sc_cpu_param = 0, process_cpuoper_cntrs = 0;
	__u32 sc_connect_ofs_idx;
	__u32 hist_param_index = 0;
	__u32 fixed_disp_pram_idx = 0;

	for (i = 0; i < vtx->num_of_subchains; i++, sc++) {

		/* work area restarts every SC: fst HW write is on ofst =0 */
		internal_context->next_wr_ofst_on_work_area = 0;

		internal_context->tds_itrtr_stat.current_sc_id_in_vtx = i;
		mbp->subch_desc->DisableBitIndP1 = 0;
		mbp->subch_desc->HwIndex = 0;
		mbp->subch_desc->CpuIndex = 0;
		sc_connect_ofs_idx = 0;
		switch(sc->stype){
		case VPUL_SUB_CH_HW:

		    current_mprb_grp_offset_in_subchain = 0;
		    stat = set_subchain_blocks_and_params(
							internal_context,
							fst_sc_pu_ptr(task, sc),
							sc->num_of_pus,
							task,
							mbp,
							&hist_param_index,
							&mbsc,
							init_mb_invoke_prm_ptr,			/* inout */
							accumulated_n_of_invoke_params_ptr,	/* inout ptr*/
							pu_location_list_ptr,
							current_offset_from_param_start,
							&sc_connect_ofs_idx,
							&fixed_disp_pram_idx,
							&current_mprb_grp_offset_in_subchain);
			if (VPU_STATUS_IS_FAILURE(stat))
				return stat;
			mbp->block_desc += sc->num_of_pus;

			set_hw_sc_connections(task, sc, mbp);

			set_hw_subchain_descriptor(internal_context, task, sc, mbp, &mbsc);
			break;
		case VPUL_SUB_CH_CPU_OP:

			sc_cpu_param = 0;
			mbp->cpu_subch_desc->FstIntRamInd   = sc->cpu.FstIntRamInd;
			mbp->cpu_subch_desc->IntRamIndices  = sc->cpu.IntRamIndices;
			internal_context->n_internal_mem   += sc->cpu.CpuRamIndices;
			mbsc.fst_intramidx = internal_context->n_internal_mem;
			mbp->cpu_subch_desc->NumCpuOp	    = sc->num_of_cpu_op;
			mbp->cpu_subch_desc->FstCpuOpOffset = process_cpuoper_cntrs;
			mbp->cpu_subch_desc->FstParamOffset = mbsc.fst_cpuoper_param;

			stat = set_cpu_subchain_cpu_ops_params(
				internal_context,
				task,
				sc->cpu.cpu_op_desc,
				sc->num_of_cpu_op,
				mbp->cpu_prm + mbsc.fst_cpuoper_param,
				&mbp->cpuop_desc,
				&sc_cpu_param);

			if (VPU_STATUS_IS_FAILURE(stat))
				return stat;

			mbsc.fst_cpuoper_param += sc_cpu_param;
			process_cpuoper_cntrs += sc->num_of_cpu_op;

			set_cpu_subchain_descriptor(task, sc, mbp, &mbsc, &cpu_subch_index);

			mbp->cpu_subch_desc++;
			cpu_subch_index++;

			break;
		default:
			VPU_ERRO("found process with invalid subchain type");
			return VPU_STATUS_ILLEGAL_TASK;
		}
		mbp->subch_desc++;
	}
	return VPU_STATUS_SUCCESS;
}

/* set process sub-chains */
static __s32 set_proc_3dnn_subchains(
				struct internal_context *internal_context,
				const struct vpul_task *task,
				struct mb_ptrs *mbp)
{
	struct vpul_vertex *vtx = vtx_ptr(task, internal_context->tds_itrtr_stat.current_vtx_id);
	struct vpul_subchain *first_subch = fst_vtx_sc_ptr(task, vtx);
	struct vpul_subchain *curr_subch = first_subch;
	struct vpul_subchain *other_subch;
	__u32 orig_seq_id;
	__u32 i, j, k;
	__u32 cpu_subch_index = 0;
	__u32 hw_3dnn_slgr_type_subch_idx = 0;
	struct mbsc_cnts mbsc = {0};
	__s32 stat;
	struct vpul_pu_location *init_pu_location_list = fst_updateble_pu_location_ptr(task);
	struct vpul_pu_location **pu_location_list_ptr = &(init_pu_location_list);
	__u16*  init_mb_invoke_prm     = (__u16 *)mbp->invoke_prm;
	__u16** init_mb_invoke_prm_ptr = &init_mb_invoke_prm; /* need pointer as an inout argument */
	__u16  accumulated_n_of_invoke_params = 0;
	__u16* accumulated_n_of_invoke_params_ptr = &accumulated_n_of_invoke_params;
	__u16 current_offset_from_param_start =
		mbp->block_prm - (mbp->first_proc_prm - VPUI_TASKPAR_ALL);
	__u32 process_cpuoper_cntrs = 0;
	__u32 sc_cpu_param = 0;
	__u32 sc_connect_ofs_idx;
	__u32 hist_param_index=0;
	__u32 fixed_disp_pram_idx;
	__u32 current_mprb_grp_offset_in_subchain;
	__u8  val;

	for (i = 0; i < vtx->num_of_subchains; i++, curr_subch++) {

		/* work area restarts every SC: fst HW write is on ofst =0 */
		internal_context->next_wr_ofst_on_work_area = 0;

		internal_context->tds_itrtr_stat.current_sc_id_in_vtx = i;
		switch(curr_subch->stype){
		case VPUL_SUB_CH_HW:
			/* all subchains sharing same seq_id must be processed in sequence */
			orig_seq_id = curr_subch->sl_group_type_ident.seq_id;
			other_subch = first_subch;
			for (j = 0; j <= i; j++, other_subch++) {
				/* check if same seq_id already encountered */
				if (other_subch->sl_group_type_ident.seq_id == orig_seq_id)
					break;
			}
			/* if j < i : this seq-id has already be processed, do nothing */
			if (j < i)
				break;
			/* 1st time encountering this seq_id : handle this subchain
			* and all other subchains with same seq_id
			*/
			set_3dnn_hw_subch_and_vpui_subch_descr(mbp, &mbsc);
			/** MPRB group offsets are relative to VPUI_Hw3DNNSubCh
			 * (not to VPUI_Hw3DNNSlGrTypeSubCh)
			 */
			current_mprb_grp_offset_in_subchain = 0;

			for (j = i; j < vtx->num_of_subchains; j++, other_subch++) {
				if (other_subch->sl_group_type_ident.seq_id == orig_seq_id) {
					sc_connect_ofs_idx = 0;
					stat = set_subchain_blocks_and_params(
						internal_context,
						fst_sc_pu_ptr(task, other_subch),
						other_subch->num_of_pus,
						task,
						mbp,
						&hist_param_index,
						&mbsc,
						init_mb_invoke_prm_ptr,			/* inout */
						accumulated_n_of_invoke_params_ptr,
										/* inout ptr*/
						pu_location_list_ptr,
						current_offset_from_param_start,
						&sc_connect_ofs_idx,
						&fixed_disp_pram_idx,
						&current_mprb_grp_offset_in_subchain);
					if (VPU_STATUS_IS_FAILURE(stat))
						return stat;
					mbp->block_desc += other_subch->num_of_pus;

					set_hw_sc_connections(task, other_subch, mbp);

					set_3dnn_hw_slgr_type_subch_descr(internal_context, task,
								vtx, other_subch,
								mbp, &mbsc,
								hw_3dnn_slgr_type_subch_idx);
					hw_3dnn_slgr_type_subch_idx++;
				}
			}
			/* replace unitialized entries in SlGrInd[] with valid values */
			for (j = 0; j < VPUI_3D_SLGR_ALL; j++) {
				val = mbp->hw_3dnn_subch_desc->SlGrInd[j];
				if (val != 0xFF) {
					for (k = 0; k < VPUI_3D_SLGR_ALL; k++) {
						if (mbp->hw_3dnn_subch_desc->SlGrInd[k] == 0xFF)
							mbp->hw_3dnn_subch_desc->SlGrInd[k] = val;
					}
				}
			}
			for (j = 0; j < VPUI_3D_SLGR_ALL; j++) {
				if (mbp->hw_3dnn_subch_desc->SlGrInd[j] == 0xFF)
					mbp->hw_3dnn_subch_desc->SlGrInd[j] = 0;
			}
			mbp->hw_3dnn_subch_desc++;
			break;
		case VPUL_SUB_CH_CPU_OP:
			mbp->subch_desc->DisableBitIndP1 = 0;
			mbp->subch_desc->CpuIndex = 0;
			mbp->subch_desc->HwIndex = 0;
			sc_cpu_param = 0;
			mbp->cpu_subch_desc->FstIntRamInd = 0;//curr_subch->cpu.fst_ram_ind;
			mbp->cpu_subch_desc->IntRamIndices = 0;// curr_subch->cpu.int_ram_ind;
			mbp->cpu_subch_desc->NumCpuOp = curr_subch->num_of_cpu_op;
			mbp->cpu_subch_desc->FstCpuOpOffset = process_cpuoper_cntrs;
			mbp->cpu_subch_desc->FstParamOffset = mbsc.fst_cpuoper_param;

			stat = set_cpu_subchain_cpu_ops_params(
							internal_context,
							task,
							curr_subch->cpu.cpu_op_desc,
							curr_subch->num_of_cpu_op,
							mbp->cpu_prm + mbsc.fst_cpuoper_param,
							&mbp->cpuop_desc, &sc_cpu_param);
			if (VPU_STATUS_IS_FAILURE(stat))
				return stat;

			mbsc.fst_cpuoper_param += sc_cpu_param;
			process_cpuoper_cntrs += curr_subch->num_of_cpu_op;

			set_cpu_subchain_descriptor(task, curr_subch, mbp,
							&mbsc, &cpu_subch_index);

			mbp->cpu_subch_desc++;
			cpu_subch_index++;
			mbp->subch_desc++;

			break;
		default:
			VPU_ERRO("found process with invalid subchain type");
			return VPU_STATUS_ILLEGAL_TASK;
		}
	}
	return VPU_STATUS_SUCCESS;
}

static void tc_set_3dnn_process(const struct vpul_vertex *vtx, struct mb_ptrs *mbp)
{
	struct VPUI_Proc3DNNDesc *mb_proc_3dnn = mbp->proc_3dnn_desc;
	const struct vpul_3dnn_process *proc_3dnn = &vtx->proc3dnn;

	mb_proc_3dnn->Base3DNNInd = proc_3dnn->base_3dnn_ind;
	mb_proc_3dnn->FstLayer = proc_3dnn->first_3dnn_layer;
	mb_proc_3dnn->LastLayer = proc_3dnn->first_3dnn_layer + proc_3dnn->num_of_3dnn_layers - 1;
	mb_proc_3dnn->Priority = proc_3dnn->Priority;
	mbp->proc_3dnn_desc++;
}

static __s32 set_pt_map_indices
(
	const struct vpul_process_inout	*io,
	struct mb_ptrs			*mbp
)
{
	__u8 * pt_map_indices = mbp->pt_map_indeces;
	__u32  i;


	if (0 != io->pts_maps_sets)
	{
		for (i = 0; i < io->n_map_indices; i++, pt_map_indices++){
			*pt_map_indices = (__u8)io->pts_map_indices[i];
		}
	}

	return VPU_STATUS_SUCCESS;
}

static __s32 set_pt_list
(
	const struct vpul_process_inout	*io,
	struct mb_ptrs			*mbp
)
{
	struct VPUI_PtList *pt_list = mbp->pt_list;
	__u32 i;

	if (0 != io->pt_list_num)
	{
		for (i = 0; i < io->pt_list_num; i++, pt_list++)
		{
			pt_list->IsDyn = 1;
			pt_list->NumInpSetsEachPt = 1;
			pt_list->FullSizesInd = i; /* image description sizes vector  index */
			pt_list->FractionBits = 16;
		}

	}

	return VPU_STATUS_SUCCESS;
}

static __s32  set_roi_from_pt
(
	const struct vpul_process_inout	*io,
	struct mb_ptrs			*mbp
)
{
	struct VPUI_RoiFromPt *roi_from_pt = mbp->roi_from_pt;
	__u32 i;

	if (0 != io->pt_list_num)
	{
		for (i = 0; i < io->pt_list_num ;i++, roi_from_pt++ )
		{
			roi_from_pt->PtListInd = i;
			roi_from_pt->SizesInd = 0;
			roi_from_pt->IsCropValid = 0;
			roi_from_pt->MapInSetInd = i;
		}
	}

	return VPU_STATUS_SUCCESS;
}

static __s32 set_pt_roi_size
(
	const struct vpul_process_inout	*io,
	struct mb_ptrs			*mbp
)
{
	__u32  i;
	struct VPUI_Sizes *pt_roi_size = mbp->pt_roi_sizes;

	if (0 != io->pt_roi_sizes)
	{
		for (i = 0; i < io->pt_roi_sizes; i++, pt_roi_size++ )
		{
			pt_roi_size->Height = io->roi_sizes_desc.roi_desc[i].roi_height;

			pt_roi_size->Width = io->roi_sizes_desc.roi_desc[i].roi_width;
		}
	}

	return VPU_STATUS_SUCCESS;
}


static __s32 set_proc_io
(
	const struct vpul_process_inout	*io,
	struct mb_ptrs			*mbp
)
 {
	__s32 stat = VPU_STATUS_SUCCESS;

	stat = set_pt_map_indices(io, mbp);
	if (VPU_STATUS_IS_FAILURE(stat))
		return stat;

	stat = set_pt_list(io, mbp);
	if (VPU_STATUS_IS_FAILURE(stat))
		return stat;

	stat = set_roi_from_pt(io, mbp);
	if (VPU_STATUS_IS_FAILURE(stat))
		return stat;

	stat = set_pt_roi_size(io, mbp);
	if (VPU_STATUS_IS_FAILURE(stat))
		return stat;

	return stat;
}

void set_proc_pre_set_mem_vec
(
	const struct vpul_task *task,
	const struct vpul_process_inout	*io,
	struct mb_ptrs			*mbp
)
{
	__u32 i;
	struct VPUI_MemVecDesc *pre_mem_vec = mbp->pre_mem_vec;

	if (0 != io->n_presets_map_desc)
	{

		for (i = 0; i < io->n_presets_map_desc; i++, pre_mem_vec++ )
		{
			pre_mem_vec->ExtMemInd	=
					io->pre_sets_mem_vec_desc[i].ext_mem_ind;
			pre_mem_vec->IsExtUpdated =
				io->pre_sets_mem_vec_desc[i].is_ext_updated;
			pre_mem_vec->IntMemType	=
				io->pre_sets_mem_vec_desc[i].int_mem_type;
			pre_mem_vec->IntMemIndex =
				io->pre_sets_mem_vec_desc[i].int_mem_index;
			pre_mem_vec->IsFixedSize =
				io->pre_sets_mem_vec_desc[i].is_fixed_size;
			pre_mem_vec->VectorBytesPerSetDiv=
				io->pre_sets_mem_vec_desc[i].vector_bytes_per_set_div;
			pre_mem_vec->DummyAlign		= 0;
		}
	}
}

void set_proc_post_set_mem_vec
(
	const struct vpul_task *task,
	const struct vpul_process_inout	*io,
	struct mb_ptrs			*mbp
)
{
	__u32 i;
	struct VPUI_MemVecDesc *post_mem_vec = mbp->post_mem_vec;

	if (0 != io->n_presets_map_desc)
	{
		for (i = 0; i < io->n_postsets_map_desc; i++, post_mem_vec++ )
		{
			post_mem_vec->ExtMemInd	=
				io->post_sets_mem_vec_desc[i].ext_mem_ind;
			post_mem_vec->IsExtUpdated =
				io->post_sets_mem_vec_desc[i].is_ext_updated;
			post_mem_vec->IntMemType	=
				io->post_sets_mem_vec_desc[i].int_mem_type;
			post_mem_vec->IntMemIndex =
				io->post_sets_mem_vec_desc[i].int_mem_index;
			post_mem_vec->IsFixedSize =
				io->post_sets_mem_vec_desc[i].is_fixed_size;
			post_mem_vec->VectorBytesPerSetDiv=
				io->post_sets_mem_vec_desc[i].vector_bytes_per_set_div;
			post_mem_vec->DummyAlign = 0;

		}
	}
}


/* fill Mbox command with one process information */
static __s32 tc_set_process(
	struct internal_context *internal_context,
	const struct vpul_task *task,
	const struct vpul_vertex *vtx,
	struct mb_ptrs *mbp,
	const struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp)
{
	__s32 stat = VPU_STATUS_SUCCESS;
	__u16 *proc_prm = mbp->first_proc_prm +
		mbp->proc_desc->Fst.Both.SubChFstIndOfs.Bl.Params;
	proc_prm[VPUI_PROC_PAR_INPUT_SETS_NUM_OFS] = vtx->proc.io.n_insets;
	proc_prm[VPUI_PROC_PAR_DISABLE_SUB_CH_MASK_OFS] = 0;

	mbp->cpu_prm = proc_prm + mbp->proc_desc->ParsOfs.CpuParamsValues;
	mbp->block_prm = proc_prm + mbp->proc_desc->ParsOfs.HwParamsValues;

	/* assumption taken for API simplicity: check assumption is correct, change API if wrong */
	BUG_ON(mb_tc->ParamsParts.ValuesOfs.Procs != VPUI_TASKPAR_ALL);

	/* set process sub-chain descriptors */
	stat = set_proc_subchains(
		internal_context,
		task,
		mbp);

	if (VPU_STATUS_IS_FAILURE(stat))
		return stat;

	/* set process inputs /outputs descriptors */
	stat = set_proc_inouts(internal_context,&vtx->proc.io, mb_tc, mb_cp, mbp);

	stat = set_proc_io(&vtx->proc.io, mbp);
	if (VPU_STATUS_IS_FAILURE(stat))
		return stat;

	set_proc_pre_set_mem_vec(task, &vtx->proc.io, mbp);
	set_proc_post_set_mem_vec(task, &vtx->proc.io, mbp);

	mbp->proc_desc++;

	return stat;
}

/* fill Mbox command with one 3DNN process base information */
static __s32 tc_set_3dnn_process_base(
	struct internal_context *internal_context,
	const struct vpul_task *task,
	const struct vpul_3dnn_process_base *proc_3dnn_base,
	struct mb_ptrs *mbp,
	const struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp)
{
	__s32 stat = VPU_STATUS_SUCCESS;

	__u16 *proc_prm = mbp->first_proc_prm +
		mbp->proc_3dnn_base_desc->Fst.Both.SubChFstIndOfs.Bl.Params;
	struct VPUI_Hw3DNNSlGrTypeSubCh *p_hw_3dnn_slgr_type_subch_desc =
		(struct VPUI_Hw3DNNSlGrTypeSubCh *)(mb_cp + mb_tc->DataOfs.Hw3DNNSlGrTypeSubCh);
	struct VPUI_Hw3DNNSubCh *p_hw_3dnn_subch =
		(struct VPUI_Hw3DNNSubCh *)(mb_cp + mb_tc->DataOfs.Hw3DNNSubCh);

	// proc_prm[VPUI_PROC_PAR_DISABLE_SUB_CH_MASK_OFS] = 0;
	mbp->hw_3dnn_slgr_type_subch_desc = p_hw_3dnn_slgr_type_subch_desc +
						mbp->proc_3dnn_base_desc->Fst.Hw3DNNSlGrTypeSubCh;
	mbp->hw_3dnn_subch_desc = p_hw_3dnn_subch + mbp->proc_3dnn_base_desc->Fst.Hw3DNNSubCh;

	mbp->cpu_prm = proc_prm + mbp->proc_3dnn_base_desc->Params3DNNOfs.CpuParamsValues;
	mbp->block_prm = proc_prm + mbp->proc_3dnn_base_desc->Params3DNNOfs.HwParamsValues;

	/* set process sub-chain descriptors */
	stat = set_proc_3dnn_subchains(internal_context, task, mbp);

	if (VPU_STATUS_IS_FAILURE(stat))
		return stat;

	set_3dnn_layers_descr_for_proc_base(task, mb_tc, proc_3dnn_base, mbp, mb_cp);

	    /* set process inputs /outputs descriptors */
	stat = set_proc_3dnn_inouts(task, proc_3dnn_base, mb_tc, mb_cp, mbp);
	/* Need to implement return value at function set_proc_inouts */
	BUG_ON_NOT_IMPLEMENTED_FUNCT(stat != VPU_STATUS_SUCCESS);

	mbp->proc_3dnn_base_desc++;

	return stat;
}

static void get_actual_cp_ptrs(
	struct mb_ptrs *pcp,
	const struct VPUI_TaskCreate *mb_tc,
	const __u8 *mb_cp,
	__u16 *mb_params
	)
{
	pcp->proc_desc = (struct VPUI_ProcDesc *)(mb_cp + mb_tc->DataOfs.Procs);
	pcp->histo_param =  (struct VPUI_HistParams *)(mb_cp + mb_tc->DataOfs.HistParams);
	pcp->pt_disp_fixed_param = (struct VPUI_DisparityFixedThr *)(mb_cp + mb_tc->DataOfs.DisparityFixedThr);
	pcp->proc_3dnn_desc = (struct VPUI_Proc3DNNDesc *)(mb_cp + mb_tc->DataOfs.Procs3DNN);
	pcp->proc_3dnn_base_desc =
			(struct VPUI_Proc3DNNBase *)(mb_cp + mb_tc->DataOfs.Procs3DNNBase);
	pcp->first_proc_prm = mb_params + mb_tc->ParamsParts.ValuesOfs.Procs;
	pcp->param_idxs = (__u16 *)(mb_cp + mb_tc->DataOfs.AllParamsIndices);
	pcp->invoke_prm = (__u16 *)(pcp->param_idxs + mb_tc->ParamsParts.Indices.TaskInvoke);
	/* indices params offsets are defined upon AllParamsIndices offset */

	pcp->subch_desc = (struct VPUI_SubCh *)(mb_cp + mb_tc->DataOfs.SubCh);
	pcp->hw_subch_desc =
		(struct VPUI_HwSubCh *)(mb_cp + mb_tc->DataOfs.HwSubCh);

	pcp->cpu_subch_desc =
		(struct VPUI_CpuSubCh *)(mb_cp + mb_tc->DataOfs.CpuSubCh);

	pcp->cpuop_desc = (struct VPUI_CpuOp *)(mb_cp + mb_tc->DataOfs.CpuOps);
	pcp->block_desc =
		(struct VPUI_BlUsage *)(mb_cp + mb_tc->DataOfs.Blocks);
	pcp->connections_desc =
		(struct VPUI_HWConnect *)(mb_cp + mb_tc->DataOfs.HwConnect);
	pcp->inout_types =
		(struct VPUI_InOutType *)(mb_cp + mb_tc->DataOfs.InOutTypes);
	pcp->roi = (struct VPUI_Roi *)(mb_cp + mb_tc->DataOfs.Rois);
	pcp->fix_map_roi =
		(struct VPUI_FixMapRoi *)(mb_cp + mb_tc->DataOfs.FixMapRois);
	pcp->size_op      = (struct VPUI_SizesOp *)
				(mb_cp + mb_tc->DataOfs.SizesOp);

	pcp->pre_mem_vec  = (struct VPUI_MemVecDesc *)
				(mb_cp + mb_tc->DataOfs.PreMemVecDesc);
	pcp->post_mem_vec = (struct VPUI_MemVecDesc *)
				(mb_cp + mb_tc->DataOfs.PostMemVecDesc);

	pcp->pt_map_indeces = (__u8 *)(mb_cp + mb_tc->DataOfs.PtMapIndices);
	pcp->pt_list =
		(struct VPUI_PtList *)(mb_cp + mb_tc->DataOfs.PtLists);

	pcp->roi_from_pt =
		(struct  VPUI_RoiFromPt *)(mb_cp + mb_tc->DataOfs.RoiFromPt);

	pcp->pt_roi_sizes =
		(struct VPUI_Sizes *)(mb_cp + mb_tc->DataOfs.PtRoiSizes);



	pcp->cnn_crop_idx    = 0;
	pcp->cnn_relu_idx    = 0;
	pcp->cnn_scaler_idx  = 1;	/* default CNN scaler is at index 0 */
	pcp->inout_types_idx = 0;
}


static void copy_coefficients( __u16 * dst, const __s32 * src, __u32 number)
{
	__u32 i;
	for (i = 0; i < number; i++,dst++, src++)
		*dst = (__u16)*src;
}

static __s32  save_coefficients_and_index_filters_info
(
	const struct vpul_task *task,
	const struct vpul_vertex *vtx,
	const struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp,
	const struct vpul_3dnn_process_base *proc_3dnn_base,
	__u32  *gfl_next_index,
	__u32  *sfl_next_index)
{

	__u32 last_static_coeffient_offset = 0;
	__u32 last_dynamic_coeffient_offset = 0;


	__u32  i, j;
	struct vpul_subchain *subchain;
	struct vpul_pu *pu;

	const struct vpul_pu_slf *slf = NULL;
	const struct vpul_pu_glf *glf = NULL;
	__s32 stat = VPU_STATUS_SUCCESS;

	const __s32* process_cofficient = NULL;

	struct VPUI_FiltCoeffsSets *mb_coeff_sep =
		(struct VPUI_FiltCoeffsSets *)(mb_cp + mb_tc->DataOfs.SepFiltCoeffSets);
	struct VPUI_FiltCoeffsSets *mb_coeff_gen  =
		(struct VPUI_FiltCoeffsSets *)(mb_cp + mb_tc->DataOfs.GenFiltCoeffSets);
	__u16 *mb_static_filters_coeff =
		(__u16 *)(mb_cp + mb_tc->DataOfs.FiltStaticCoeffs);

	__u16 *mb_params  = (__u16 *)(mb_cp + mb_tc->DataOfs.AllParamsValues);
	__u16 *mb_dyn_filters_coeff =
			(__u16 *)(mb_params + mb_tc->ParamsParts.ValuesOfs.DynFiltCoeffs);

	if (vtx->vtype == VPUL_VERTEXT_3DNN_PROC) {
		process_cofficient = proc_3dnn_base->io.static_coff;
	}
	else{
		BUG_ON(vtx->vtype != VPUL_VERTEXT_PROC);
		process_cofficient = vtx->proc.io.static_coff;
	}

	subchain = fst_vtx_sc_ptr(task, vtx);

	for (j = 0; j < vtx->num_of_subchains; j++, subchain++) {
		if (subchain->stype != VPUL_SUB_CH_CPU_OP){
			pu = fst_sc_pu_ptr(task, subchain);
			for (i = 0; i < subchain->num_of_pus; i++, pu++) {
				if (pu->op_type == VPUL_OP_SEP_FLT) {
					slf = &(pu->params.slf);
					mb_coeff_sep[*sfl_next_index].FlBits =
						slf->vertical_filter_coeff.vsize<< FL_EACH_DIM_SIZE_LSB |
						slf->vertical_filter_coeff.is_dynamic << FL_IS_DYN_LSB  |
						slf->coefficient_fraction  << FL_COEF_FRAC_BITS_LSB;

					if (0 == slf->vertical_filter_coeff.is_dynamic){
						copy_coefficients(
							mb_static_filters_coeff+last_static_coeffient_offset,
							&process_cofficient[slf->vertical_filter_coeff.offset],
							slf->vertical_filter_coeff.vsize*2);
						mb_coeff_sep[*sfl_next_index].FstCoeffOfs =
							last_static_coeffient_offset;
						//update index:
						last_static_coeffient_offset += slf->vertical_filter_coeff.vsize * 2;
					}
					else
					{
						copy_coefficients(
							mb_dyn_filters_coeff+last_dynamic_coeffient_offset,
							&process_cofficient[slf->vertical_filter_coeff.offset],
							slf->vertical_filter_coeff.vsize*2);
						mb_coeff_sep[*sfl_next_index].FstCoeffOfs =
							last_dynamic_coeffient_offset;
						//update index:
						last_dynamic_coeffient_offset += slf->vertical_filter_coeff.vsize * 2;
					}
					(*sfl_next_index)++;

				}
				if (pu->op_type == VPUL_OP_GEN_FLT) {
					__u32 num_of_coefficients;
					glf = &(pu->params.glf);
					num_of_coefficients = glf->filter_coeff.vsize*glf->filter_coeff.vsize;
					mb_coeff_gen[*gfl_next_index].FlBits =
							 glf->filter_coeff.vsize << FL_EACH_DIM_SIZE_LSB    |
							 glf->two_outputs << FL_IS_GEN_2FILTERS_LSB         |
							 glf->filter_coeff.is_dynamic << FL_IS_DYN_LSB      |
							 glf->coefficient_fraction << FL_COEF_FRAC_BITS_LSB |
							 glf->signed_coefficients << FL_GEN_COEF_FRAC_SIGN_LSB;

					if(glf->two_outputs!=0)
						num_of_coefficients *= 2;
					if (0 == glf->filter_coeff.is_dynamic){
						copy_coefficients(
							mb_static_filters_coeff+last_static_coeffient_offset,
							&process_cofficient[glf->filter_coeff.offset],
							num_of_coefficients);
						mb_coeff_gen[*gfl_next_index].FstCoeffOfs =
							last_static_coeffient_offset;
						//update index:
						last_static_coeffient_offset += num_of_coefficients;
					}
					else
					{
						copy_coefficients(
							mb_dyn_filters_coeff+last_dynamic_coeffient_offset,
							&process_cofficient[glf->filter_coeff.offset],
							num_of_coefficients);
							mb_coeff_gen[*gfl_next_index].FstCoeffOfs =
							last_dynamic_coeffient_offset;
						//update index:
						last_dynamic_coeffient_offset += num_of_coefficients;
					}
					(*gfl_next_index)++;

				}
			}
		}
	}

	return stat;
}

static void save_tot_ops_info(
	const struct vpul_task *task,
	const struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp)
{
	__u32  i, j, k;
	__u32 num_scales;
	__u32 num_crops;
	const struct vpul_vertex *vtx = fst_vtx_ptr(task);
	const struct vpul_3dnn_layer *layer_3dnn;
	__u8 *mb_ptr_for_even_addr_align;
	const struct vpul_3dnn_process_base *proc_3dnn_base = fst_3dnn_process_base_ptr(task);
	const struct VPUI_Proc3DNNBase *mb_proc_desc =
			(struct VPUI_Proc3DNNBase *)(mb_cp + mb_tc->DataOfs.Procs3DNNBase);

	struct VPUI_CropOp  *pCropOps = (struct VPUI_CropOp *)(mb_cp + mb_tc->DataOfs.CropOps);
	struct VPUI_ScaleOp *pScaleOps = (struct VPUI_ScaleOp *)(mb_cp + mb_tc->DataOfs.ScaleOps);
	__u8 *mb_crop_ind_vec = mb_cp + mb_tc->DataOfs.CropInd;
	__u8 *mb_scale_ind_vec = mb_cp + mb_tc->DataOfs.ScaleInd;
	__u8 mb_crop_ind = 0;
	__u8 mb_scale_ind = 0;

	num_scales = 0;
	num_crops = 0;
	for (i = 0; i < task->t_num_of_vertices; i++, vtx++) {
		if (vtx->vtype == VPUL_VERTEXT_PROC) {
			/* Croppers sizes */
			for (j = 0; j < vtx->proc.io.n_croppers; ++pCropOps, j++) {
				pCropOps->Right = vtx->proc.io.croppers[j].Right;
				pCropOps->Left = vtx->proc.io.croppers[j].Left;
				pCropOps->Top = vtx->proc.io.croppers[j].Top;
				pCropOps->Bottom = vtx->proc.io.croppers[j].Bottom;
			}
			num_crops += vtx->proc.io.n_croppers;

			/* Scales sizes */
			for (j = 0; j < vtx->proc.io.n_scales; ++pScaleOps, j++) {
				pScaleOps->WidthNumer =
						vtx->proc.io.scales[j].horizontal.numerator;
				pScaleOps->WidthDenom =
						vtx->proc.io.scales[j].horizontal.denominator;
				pScaleOps->HeightNumer =
						vtx->proc.io.scales[j].vertical.numerator;
				pScaleOps->HeightDenom =
						vtx->proc.io.scales[j].vertical.denominator;
			}
			/* "Fix" not implement in TDS  */
			num_scales += vtx->proc.io.n_scales;
		}
	}

	for (i = 0; i < task->t_num_of_3dnn_process_bases; i++, proc_3dnn_base++, mb_proc_desc++) {
		mb_crop_ind = mb_proc_desc->Fst.CropInd;
		mb_scale_ind = mb_proc_desc->Fst.ScaleInd;
		for (j = 0; j < proc_3dnn_base->number_of_layers; j++) {
			layer_3dnn = &proc_3dnn_base->layers[j];
			/* Cropper sizes */
			for (k = 0; k < proc_3dnn_base->io.n_croppers; ++pCropOps, k++) {
				pCropOps->Right =layer_3dnn->croppers[j].Right;
				pCropOps->Left = layer_3dnn->croppers[j].Left;
				pCropOps->Top = layer_3dnn->croppers[j].Top;
				pCropOps->Bottom = layer_3dnn->croppers[j].Bottom;
				mb_crop_ind_vec[mb_crop_ind] = num_crops;
				mb_crop_ind++;
				num_crops++;
			}

			/* Scale sizes */
			for (k = 0; k < proc_3dnn_base->io.n_scales; ++pScaleOps, k++) {
				pScaleOps->WidthNumer = layer_3dnn->scales[k].horizontal.numerator;
				pScaleOps->WidthDenom =
						layer_3dnn->scales[k].horizontal.denominator;
				pScaleOps->HeightNumer = layer_3dnn->scales[k].vertical.numerator;
				pScaleOps->HeightDenom =
						layer_3dnn->scales[k].vertical.denominator;
				mb_scale_ind_vec[mb_scale_ind] = num_scales;
				mb_scale_ind++;
				num_scales++;
			}
		}
	}
	mb_ptr_for_even_addr_align = &mb_crop_ind_vec[mb_crop_ind];
	PAD_U8_PTR_TO_ALIGN(mb_ptr_for_even_addr_align);
	mb_ptr_for_even_addr_align = &mb_scale_ind_vec[mb_scale_ind];
	PAD_U8_PTR_TO_ALIGN(mb_ptr_for_even_addr_align);
}

/* return value : number of items written to HwReadResBl vector */
static __u32 set_hw_res_block_id(
	const struct vpul_task *task,
	const struct vpul_vertex *vtx,
	const struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp,
	__u8 *mb_hw_res_ops)
{
	__u32 num_entries_written = 0;
	__u32  i, j;
	struct vpul_subchain *subchain;
	struct vpul_pu *pu;
	const struct vpul_pu *first_pu;

	subchain = fst_vtx_sc_ptr(task, vtx);
	for (j = 0; j < vtx->num_of_subchains; j++, subchain++){
		if (subchain->stype != VPUL_SUB_CH_CPU_OP){
			pu = fst_sc_pu_ptr(task, subchain);
			first_pu = pu;
			for (i = 0; i < subchain->num_of_pus; i++,pu++ ) {
				/* get offset of hw-results-generating PU's,
				 * written to HwReadResBl
				 */
				if (pu_num_of_hw_res_to_read(pu)) {
					mb_hw_res_ops[num_entries_written] = (__u8)(pu - first_pu);
					num_entries_written++;
				}
			}
		}
	}
	return num_entries_written;
}

/* fill Mbox command parameter descriptors with all processes information */
static __s32 set_process_level_descriptors(
	struct internal_context  * internal_context,
	const struct vpul_task *task,
	struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp)
{
	struct mb_ptrs mbp;
	__u32 i;
	const struct vpul_vertex *first_vtx = fst_vtx_ptr(task);
	const struct vpul_vertex *vtx = first_vtx;
	const struct vpul_3dnn_process_base *proc_3dnn_base = fst_3dnn_process_base_ptr(task);
	__u16 *mb_params  = (__u16 *)(mb_cp + mb_tc->DataOfs.AllParamsValues);
	__u32  gfl_next_index = 0;
	__u32  sfl_next_index = 0;
	__u8 *mb_hw_res_ops = (__u8 *)(mb_cp + mb_tc->DataOfs.HwReadResBl);


	__s32 stat = VPU_STATUS_SUCCESS;
	__s32 stat_coef = VPU_STATUS_SUCCESS;

	/* initialize internal_context fields */
	internal_context->tds_itrtr_stat.num_of_updatable_pus_remained = task->t_num_of_pu_params_on_invoke;
	internal_context->tds_itrtr_stat.next_roi_index = 0;

	mb_params[VPUI_TASKPAR_DISABLE_EDGES_MASK_OFS] = 0;

	save_tot_ops_info(task, mb_tc, mb_cp);

	/* set mbox pointers to parameters and descriptor */
	get_actual_cp_ptrs(&mbp, mb_tc, mb_cp, mb_params);
	for (i = 0; i < task->t_num_of_vertices; i++, vtx++) {
		internal_context->tds_itrtr_stat.current_vtx_id = i;
		switch (vtx->vtype) {
		case VPUL_VERTEXT_PROC:
			stat = tc_set_process(internal_context,
						task, vtx, &mbp, mb_tc,
						mb_cp);
			stat_coef = save_coefficients_and_index_filters_info
						(task, vtx, mb_tc, mb_cp, proc_3dnn_base,
						 &gfl_next_index, &sfl_next_index);

			mb_hw_res_ops += set_hw_res_block_id(task,
							vtx,
							mb_tc,
							mb_cp,
							mb_hw_res_ops);
			break;
		case VPUL_VERTEXT_3DNN_PROC:
			tc_set_3dnn_process(vtx, &mbp);
			break;
		case VPUL_VERTEXT_HOST_REP:
		case VPUL_VERTEXT_END:
			/* TODO : handle host reports */
			// set output: required_output_mem_allc_16B
			if (internal_context->required_output_mem_allc_16B>0)
			{
				struct VPUI_TaskVertex *mb_vtx =
					(struct VPUI_TaskVertex *)(mb_cp + mb_tc->DataOfs.Vertex);

				mb_vtx[i].FstParamOfs = 0; /* currently, only first output slot implemented and used*/
				mb_vtx[i].TotalParams = internal_context->required_output_mem_allc_16B;
			}

			break;
		default:
			break;
		}

		if ((VPU_STATUS_IS_FAILURE(stat))		||
			(VPU_STATUS_IS_FAILURE(stat_coef)))
			break;
	}

	for (i = 0; i < task->t_num_of_3dnn_process_bases; i++, proc_3dnn_base++) {
		vtx = vertex_referencing_this_3dnn_proc_base(task, i);
		internal_context->tds_itrtr_stat.current_vtx_id = vtx - first_vtx;
		if (vtx) {
			stat = tc_set_3dnn_process_base(internal_context,
						task, proc_3dnn_base, &mbp, mb_tc,
						mb_cp);
			stat_coef = save_coefficients_and_index_filters_info(task, vtx,
									mb_tc, mb_cp,
									proc_3dnn_base,
									&gfl_next_index,
									&sfl_next_index);

			mb_hw_res_ops += set_hw_res_block_id(task,
							vtx,
							mb_tc,
							mb_cp,
							mb_hw_res_ops);
		}
	}
	/* If we had an uneven number of PU we need to fill the last byte
	   (For consistency) */
	PAD_U8_PTR_TO_ALIGN(mb_hw_res_ops);
	/* return value must be even - is it is odd : add 1 */
	mb_tc->Totals.OutputTotal16BSize = internal_context->required_output_mem_allc_16B;
	if (internal_context->required_output_mem_allc_16B>0)
		mb_tc->Totals.OutputOffsets = 1;
	else
		mb_tc->Totals.OutputOffsets = 0; /* value is don't care, set 0.*/

	return stat;
}

static __u32 get_num_of_coeffs_for_slice_group(const struct vpul_3dnn_layer *layer_3dnn,
							__u32 filt_size,
							__u32 img_xy_size,
							__u32 total_number_of_coeffs_dma,
							__u32 number_of_slices)
{
	__u32 ret_val = number_of_slices;

	if (!layer_3dnn->cnn_descr.is_dot_mode)
		ret_val *= filt_size;
	if (layer_3dnn->cnn_descr.is_location_dependent_coeffs)
		ret_val *= img_xy_size;
	if (layer_3dnn->cnn_descr.is_filter_bias_in_coeffs_mem)
		ret_val += 2;
	ret_val *= layer_3dnn->n_output_slices;
	ret_val = (ret_val + total_number_of_coeffs_dma - 1) / total_number_of_coeffs_dma;
	return ret_val;
}

static void set_3dnn_layer_desc_XY_sizes(const struct vpul_3dnn_layer *layer_3dnn,
					__u32 index,
					struct VPUI_3DNNIoXYSizes *mb_3dnn_xy_sizes)
{
	__u32 filter_size;
	__u32 image_xy_size;
	__u32 num_coeffs_not_last_slice_group;
	__u32 num_coeffs_in_last_slice_group;
	__u32 num_slices_in_last_slice_group;
	__u32 total_num_coeffs_dma;	/* counting 32-bit DMA as 2 DMAs */

	mb_3dnn_xy_sizes[index + VPUI_XYSIZE_3DNN_INPUT].XDimSize = layer_3dnn->dim_size_input.x;
	mb_3dnn_xy_sizes[index + VPUI_XYSIZE_3DNN_INPUT].YDimSize = layer_3dnn->dim_size_input.y;
	if (layer_3dnn->n_slices_per_input_slice_group == layer_3dnn->n_total_input_slices) {
		/* single slgr : no intermediate size, output size = last slgr output size */
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_3DNN_OUTPUT].XDimSize =
						layer_3dnn->dim_size_last_or_single_slgr_output.x;
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_3DNN_OUTPUT].YDimSize =
						layer_3dnn->dim_size_last_or_single_slgr_output.y;
	} else {
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_3DNN_OUTPUT].XDimSize =
						layer_3dnn->dim_size_intermediate_output.x;
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_3DNN_OUTPUT].YDimSize =
						layer_3dnn->dim_size_intermediate_output.y;
	}
	mb_3dnn_xy_sizes[index + VPUI_XYSIZE_LAST_SLGR_OUTPUT].XDimSize =
						layer_3dnn->dim_size_last_or_single_slgr_output.x;
	mb_3dnn_xy_sizes[index + VPUI_XYSIZE_LAST_SLGR_OUTPUT].YDimSize =
						layer_3dnn->dim_size_last_or_single_slgr_output.y;

	/* calculate number of coefficients per DMA for last slice group and
	 * not last slice group
	 */
	/* total number of DMAs for coefficients : 32-bit DMAs counted as 2 DMAs */
	total_num_coeffs_dma = layer_3dnn->cnn_descr.number_of_16_bit_dmas_for_coeffs +
			(layer_3dnn->cnn_descr.number_of_32_bit_dmas_for_coeffs << 1);
	filter_size = layer_3dnn->cnn_descr.filter_width *
						layer_3dnn->cnn_descr.filter_height;
	image_xy_size = layer_3dnn->dim_size_input.x * layer_3dnn->dim_size_input.y;
	num_coeffs_not_last_slice_group = get_num_of_coeffs_for_slice_group(
						layer_3dnn,
						filter_size,
						image_xy_size,
						total_num_coeffs_dma,
						layer_3dnn->n_slices_per_input_slice_group);
	num_slices_in_last_slice_group = layer_3dnn->n_total_input_slices %
					layer_3dnn->n_slices_per_input_slice_group;
	if (num_slices_in_last_slice_group == 0)
		num_slices_in_last_slice_group = layer_3dnn->n_slices_per_input_slice_group;
	num_coeffs_in_last_slice_group = get_num_of_coeffs_for_slice_group(
						layer_3dnn,
						filter_size,
						image_xy_size,
						total_num_coeffs_dma,
						num_slices_in_last_slice_group);
	/* translate number of coefficients to value X * Y
	 * X should be as large as possible but not exceed 16-bit DMA capacity
	 * which is 8 Kbytes (4K 16-bit values)
	 */
	if (num_coeffs_not_last_slice_group <= 4096) {
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_COEFFS].XDimSize =
					num_coeffs_not_last_slice_group;
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_COEFFS].YDimSize = 1;
	} else {
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_COEFFS].XDimSize = 4096;
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_COEFFS].YDimSize =
				(num_coeffs_not_last_slice_group + 4095) >> 12;
	}
	if (num_coeffs_in_last_slice_group <= 4096) {
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_LAST_SLGR_COEFFS].XDimSize =
					num_coeffs_in_last_slice_group;
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_LAST_SLGR_COEFFS].YDimSize = 1;
	} else {
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_LAST_SLGR_COEFFS].XDimSize = 4096;
		mb_3dnn_xy_sizes[index + VPUI_XYSIZE_LAST_SLGR_COEFFS].YDimSize =
				(num_coeffs_in_last_slice_group + 4095) >> 12;
	}
}

static __u8 cnn_cfg_val(const struct vpul_3dnn_layer *layer_3dnn)
{
	__u8 cnn_cfg = (layer_3dnn->cnn_descr.is_dot_mode << CNN_CFG_IS_DOT_MODE_LSB) |
			(layer_3dnn->cnn_descr.is_pass_thru_mode << CNN_CFG_IS_PASSTH_MODE_LSB) |
			(layer_3dnn->cnn_descr.is_slice_sum << CNN_CFG_IS_SLICE_SUM_LSB) |
			(layer_3dnn->cnn_descr.is_filter_bias_in_coeffs_mem
						<< CNN_CFG_IS_FILT_BIAS_MEM_LSB) |
			(layer_3dnn->cnn_descr.is_filter_bias_used
						<< CNN_CFG_IS_FILT_BIAS_USED_LSB) |
			(layer_3dnn->cnn_descr.is_relu_enabled << CNN_CFG_IS_RELU_ENABLED_LSB) |
			(layer_3dnn->cnn_descr.is_location_dependent_coeffs
						<< CNN_CFG_IS_LOC_DEP_COEFFS_LSB);

	if (layer_3dnn->cnn_descr.xy_crop != VPU_CNN_XY_CROP_NONE)
		cnn_cfg |= (1 << CNN_CFG_IS_XY_CROP_LSB);

	return cnn_cfg;
}

static __u8 cnn_dma_bits(const struct vpul_3dnn_layer *layer_3dnn)
{
	return ((layer_3dnn->cnn_descr.number_of_16_bit_dmas_for_coeffs <<
						CNN_DMA_COEFFS_16B_NUM_LSB) |
		(layer_3dnn->cnn_descr.number_of_32_bit_dmas_for_coeffs <<
						CNN_DMA_COEFFS_32B_NUM_LSB) |
		(layer_3dnn->cnn_descr.is_16_bit_dma_for_data_input <<
						CNN_DMA_IS_INPUT_16B_LSB) |
		(layer_3dnn->cnn_descr.is_16_bit_dma_for_accum_data <<
						CNN_DMA_IS_ACC_16B_LSB));
}

static __u8 cnn_format_bits(const struct vpul_3dnn_layer *layer_3dnn)
{
	return ((layer_3dnn->cnn_descr.input_format << CNN_FORMAT_INPUT_LSB) |
		(layer_3dnn->cnn_descr.output_format << CNN_FORMAT_OUTPUT_LSB) |
		(layer_3dnn->cnn_descr.is_temporary_slice_group_results_16f <<
							CNN_FORMAT_SLGR_16F_LSB) |
		(layer_3dnn->cnn_descr.is_external_accumulated_data <<
							CNN_FORMAT_IS_ACC_DATA_LSB) |
		(layer_3dnn->cnn_descr.is_extern_acc_data_16f << CNN_FORMAT_ACC_16F_LSB));
}

static void set_cnn_relu_desc(const struct vpul_3dnn_layer *layer_3dnn,
			struct VPUI_CnnReluDesc *mb_cnn_relu_desc)
{
	mb_cnn_relu_desc->Neg.Bits =
			(layer_3dnn->cnn_descr.relu_desc.neg.sign << CNN_RELU_SIGN_LSB) |
			(layer_3dnn->cnn_descr.relu_desc.neg.exponent << CNN_RELU_EXPONENET_LSB) |
			((layer_3dnn->cnn_descr.relu_desc.neg.mantissa &
				    CNN_RELU_MANT_LSBS_MASK) << CNN_RELU_MANT_LSBS_LSB);
	mb_cnn_relu_desc->Neg.MantissaMsbs = layer_3dnn->cnn_descr.relu_desc.neg.mantissa >> 2;
	mb_cnn_relu_desc->Pos.Bits =
			(layer_3dnn->cnn_descr.relu_desc.pos.sign << CNN_RELU_SIGN_LSB) |
			(layer_3dnn->cnn_descr.relu_desc.pos.exponent << CNN_RELU_EXPONENET_LSB) |
			((layer_3dnn->cnn_descr.relu_desc.pos.mantissa &
				    CNN_RELU_MANT_LSBS_MASK) << CNN_RELU_MANT_LSBS_LSB);
	mb_cnn_relu_desc->Pos.MantissaMsbs = layer_3dnn->cnn_descr.relu_desc.pos.mantissa >> 2;
}

static void set_cnn_cropper_desc(const struct vpul_3dnn_layer *layer_3dnn,
				struct VPUI_CnnCropDesc *mb_cnn_cropper)
{
	mb_cnn_cropper->Bottom = layer_3dnn->cnn_descr.crop_desc.bottom;
	mb_cnn_cropper->Left = layer_3dnn->cnn_descr.crop_desc.left;
	mb_cnn_cropper->Right = layer_3dnn->cnn_descr.crop_desc.right;
	mb_cnn_cropper->Top = layer_3dnn->cnn_descr.crop_desc.top;
}

/* fill Mbox command with 3DNN layers information for specified 3DNN process base */
static void set_3dnn_layers_descr_for_proc_base(const struct vpul_task *task,
						const struct VPUI_TaskCreate *mb_tc,
						const struct vpul_3dnn_process_base *proc_base,
						struct mb_ptrs *mbp, __u8 *mb_cp)
{
	__u32 i, j;
	__u32 io_xy_3dnn_sizes_index = 0;

	const struct vpul_3dnn_layer *layer_3dnn;
	const struct VPUI_Proc3DNNBase *mb_proc_base = mbp->proc_3dnn_base_desc;
	struct VPUI_3DNNLayer *mb_layer_vec =
		(struct VPUI_3DNNLayer *)(mb_cp + mb_tc->DataOfs.Layers3DNN);
	struct VPUI_3DNNLayer *mb_layer = &mb_layer_vec[mb_proc_base->Fst.Layers3DNN];

	struct VPUI_CnnCropDesc *mb_cnn_cropper_vec =
		(struct VPUI_CnnCropDesc *)(mb_cp + mb_tc->DataOfs.CnnCropDesc);
	struct VPUI_CnnReluDesc *mb_cnn_relu_desc_vec =
		(struct VPUI_CnnReluDesc *)(mb_cp + mb_tc->DataOfs.ReluDesc);

	struct VPUI_3DNNIoXYSizes *mb_3dnn_xy_sizes_vec =
		(struct VPUI_3DNNIoXYSizes *)(mb_cp + mb_tc->DataOfs.IoXY3DNNSizes);
	struct VPUI_3DNNIoXYSizes *mb_3dnn_xy_sizes =
		&mb_3dnn_xy_sizes_vec[mb_proc_base->Fst.IoXY3DNNSizes];
	__u16 *mb_cnn_scaler = (__u16 *)(mb_cp + mb_tc->DataOfs.CNNScalers);

	mb_cnn_scaler[0] = DEFAULT_CNN_SCALER;

	for (i = 0; i < proc_base->number_of_layers; i++, mb_layer++) {
		layer_3dnn = &proc_base->layers[i];

		mb_layer->CoeffsSlGrOfsBytesDma16Lsbs =
			(__u16)(layer_3dnn->coeff_size_per_slice_group_dma16 & 0xFFFF);
		mb_layer->CoeffsSlGrOfsBytesDma16Msbs =
			(__u16)(layer_3dnn->coeff_size_per_slice_group_dma16 >> 16);

		mb_layer->CoeffsSlGrOfsBytesDma32Lsbs =
			(__u16)(layer_3dnn->coeff_size_per_slice_group_dma32 & 0xFFFF);
		mb_layer->CoeffsSlGrOfsBytesDma32Msbs =
			(__u16)(layer_3dnn->coeff_size_per_slice_group_dma32 >> 16);

		mb_layer->InputTileIndP1 = 0;
		mb_layer->OutputTileIndP1 = 0;
		mb_layer->LastSlGrOutTileIndP1 = 0;
		mb_layer->CnnDesc.CropIndP1 = 0;
		mb_layer->CnnDesc.CfgBits = cnn_cfg_val(layer_3dnn);
		if (layer_3dnn->cnn_descr.xy_crop == VPU_CNN_XY_CROP_EXPLICIT) {
			set_cnn_cropper_desc(layer_3dnn, &mb_cnn_cropper_vec[mbp->cnn_crop_idx]);
			mbp->cnn_crop_idx++;
			mb_layer->CnnDesc.CropIndP1 = mbp->cnn_crop_idx;
		}
		mb_layer->CnnDesc.ReluInd = 0;
		if (layer_3dnn->cnn_descr.is_relu_enabled) {
			set_cnn_relu_desc(layer_3dnn, &mb_cnn_relu_desc_vec[mbp->cnn_relu_idx]);
			mb_layer->CnnDesc.ReluInd = mbp->cnn_relu_idx;
			mbp->cnn_relu_idx++;
		}
		mb_layer->FstInputFlScaleFactor = 0;
		mb_layer->LastInputFlScaleFactor = 0;
		if (layer_3dnn->n_scale_factors) {
			for (j = 0; j < layer_3dnn->n_scale_factors; j++)
				mb_cnn_scaler[mbp->cnn_scaler_idx + j] =
					(__u16)(layer_3dnn->scale_factors[j]);
			mb_layer->FstInputFlScaleFactor = mbp->cnn_scaler_idx;
			mbp->cnn_scaler_idx += layer_3dnn->n_scale_factors;
			mb_layer->LastInputFlScaleFactor = mbp->cnn_scaler_idx - 1;
		}
		mb_layer->CnnDesc.FilterSizesBits =
				(layer_3dnn->cnn_descr.filter_width << CNN_FILTER_WIDTH_LSB) |
				(layer_3dnn->cnn_descr.filter_height << CNN_FILTER_HEIGHT_LSB);
		mb_layer->CnnDesc.DmaBits = cnn_dma_bits(layer_3dnn);
		mb_layer->CnnDesc.FormatBits = cnn_format_bits(layer_3dnn);
		mb_layer->TotInputSlicesNum = layer_3dnn->n_total_input_slices;
		mb_layer->GroupInputSlicesNum = layer_3dnn->n_slices_per_input_slice_group;
		mb_layer->TotOutputSlicesNum = layer_3dnn->n_output_slices;
		set_3dnn_layer_desc_XY_sizes(layer_3dnn,
					io_xy_3dnn_sizes_index,
					mb_3dnn_xy_sizes);
		for (j = 0; j < VPUI_XYSIZE_ALL; j++)
			mb_layer->IoXySizesIndices[j] = io_xy_3dnn_sizes_index + j;
		io_xy_3dnn_sizes_index += VPUI_XYSIZE_ALL;
	}
}

/* found number of processes sub-chains elements */
__s32 set_proc_sub_chains_totals(
	const struct vpul_task *task,
	const struct vpul_vertex *vtx,
	struct	VPUI_ProcTotals *ptotals,
	struct	VPUI_ProcParamsOfs *pofs
	)
{
	struct vpul_subchain *sc = fst_vtx_sc_ptr(task, vtx);
	const struct vpul_pu *pu = NULL;
	struct vpul_cpu_op *cpu_oper = NULL;
	struct VPUI_AllTotals *pall = &ptotals->All;
	__u32 cpuop_params = 0;
	__u32 i,j;
	__u32 isFirstHwSubChain = 1;

	ptotals->Task.HwSubCh = 0;
	pall->HwSubCh.Blocks = 0;
	pall->HwSubCh.HwConnections = 0;
	pall->HwSubCh.HwMprbsIndGr = 0;
	pall->HwSubCh.InternalRamsInd = count_proc_int_ram_desc( task, vtx);
	pall->HwSubCh.HwReadResBl = 0;
	pall->CpuOps = 0;
	pall->SubCh = 0;
	pall->ParamsIndices = 0;
	pall->CpuSubCh = 0;

	for (i = 0; i < vtx->num_of_subchains; i++, sc++) {
		switch (sc->stype) {
		case VPUL_SUB_CH_HW:
			if (isFirstHwSubChain){
				pu = fst_sc_pu_ptr(task, sc);
				isFirstHwSubChain = 0;
			}

			ptotals->Task.HwSubCh++;
			pall->HwSubCh.Blocks += sc->num_of_pus;
			break;
		case VPUL_SUB_CH_CPU_OP:
			cpu_oper = &sc->cpu.cpu_op_desc[0];

			/* add CPU OPER parameters */
			for (j = 0; j < sc->num_of_cpu_op; j++, cpu_oper++){
				cpuop_params += get_cpuop_param_number(cpu_oper);
			}

			pall->CpuOps += sc->num_of_cpu_op;
			pall->CpuSubCh++;
			break;
		default:
			VPU_ERRO("found process with invalid subchain type");
			return VPU_STATUS_ILLEGAL_TASK;
		}
		pall->SubCh++;
	}

	/* add HW blocks parameters */
	if (pu) {
		for (i = 0; i < pall->HwSubCh.Blocks; i++, pu++) {
			pall->HwSubCh.HwConnections += pu->n_in_connect;
			pall->HwSubCh.HwMprbsIndGr  += pu_get_num_mprb_groups(pu);
			pall->ParamsValues          += pu_get_num_params(pu);
			pall->HwSubCh.HwReadResBl += (pu_num_of_hw_res_to_read(pu) != 0);
		}
	}

	pofs->CpuParamsValues = pall->ParamsValues;
	pall->ParamsValues += cpuop_params;

	return VPU_STATUS_SUCCESS;
}

__s32 set_3dnn_proc_sub_chains_totals(
		const struct vpul_task *task,
		const struct vpul_vertex *vtx,
		struct	VPUI_Proc3DNNTotals *ptotals,
		struct	VPUI_3DNNParamsOfs *pofs)
{
	__u32 num_subch = vtx->num_of_subchains;
	struct vpul_subchain *sc = fst_vtx_sc_ptr(task, vtx);
	struct vpul_subchain *first_subchain = sc;
	struct vpul_subchain *sc0;
	const struct vpul_pu *pu = fst_sc_pu_ptr(task, first_subchain);
	struct VPUI_AllTotals *pall = &ptotals->All;
	__u32 cpuop_params = 0;
	__u32 i, j;
	__u32 orig_seq_id;

	ptotals->Task.Hw3DNNSubCh = 0;
	ptotals->Task.Hw3DNNSlGrTypeSubCh = 0;
	pall->HwSubCh.Blocks = 0;
	pall->HwSubCh.HwConnections = 0;
	pall->HwSubCh.HwMprbsIndGr = 0;
	pall->HwSubCh.InternalRamsInd = 0;
	pall->HwSubCh.HwReadResBl = 0;
	pall->CpuOps = 0;
	pall->SubCh = 0;
	pall->ParamsIndices = 0;
	pall->CpuSubCh = 0;

	for (i = 0; i < num_subch; i++, sc++) {
		switch (sc->stype) {
		case VPUL_SUB_CH_HW:
			ptotals->Task.Hw3DNNSlGrTypeSubCh++;
			sc0 = first_subchain;
			orig_seq_id = sc->sl_group_type_ident.seq_id;
			/* check if already counted as Hw3DNNSubCh
			 * (same seq_id already encountered)
			 */
			for (j = 0; j < num_subch; j++, sc0++) {
				if (sc0->sl_group_type_ident.seq_id == orig_seq_id)
					break;
			}
			if (j == i) {
				ptotals->Task.Hw3DNNSubCh++;
				pall->SubCh++;
			}

			pall->HwSubCh.Blocks += sc->num_of_pus;
			break;
		case VPUL_SUB_CH_CPU_OP:
			VPU_ERRO("found 3dnn process with CPU operation sub-chain");
			return VPU_STATUS_ILLEGAL_TASK;
			break;
		default:
			VPU_ERRO("found process with invalid subchain type");
			return VPU_STATUS_ILLEGAL_TASK;
		}
	}

	/* add HW blocks parameters */
	for (i = 0; i < pall->HwSubCh.Blocks; i++, pu++) {
		pall->HwSubCh.HwConnections += pu->n_in_connect;
		pall->HwSubCh.HwMprbsIndGr += pu_get_num_mprb_groups(pu);
		pall->ParamsValues += pu_get_num_params(pu);
		pall->HwSubCh.HwReadResBl += (pu_num_of_hw_res_to_read(pu) != 0);
	}

	pofs->CpuParamsValues = pall->ParamsValues;
	pall->ParamsValues += cpuop_params;

	return VPU_STATUS_SUCCESS;
}

static void set_proc_loops_totals(
	const struct vpul_process_inout *pio,
	struct	VPUI_ProcTotals *ptotals,
	struct	VPUI_ProcParamsOfs *pofs)
{
	struct VPUI_AllTotals *pall = &ptotals->All;
	const struct vpul_loop_totals *proc_loop = &pio->param_loop;
	struct VPUI_ProcOnlyTotals *ponly = &ptotals->Only;
	struct VPUI_LoopParamsOfs *loop_ofs = &pofs->LoopParams;
	pofs->LoopParamsValues = pall->ParamsValues;
	loop_ofs->PerSet.FstVal = 0;
	loop_ofs->PerSet.FstInd = 0;
	loop_ofs->FixUpdate = pofs->LoopParams.PerSet;
	loop_ofs->FixUpdate.FstVal += proc_loop->n_sets *
		proc_loop->n_exp_per_sets;
	loop_ofs->FixUpdate.FstInd += proc_loop->n_exp_per_sets;
	loop_ofs->FstLoop = pofs->LoopParams.FixUpdate;
	loop_ofs->FstLoop.FstVal += proc_loop->fix_inc * 2;
	loop_ofs->FstLoop.FstInd += proc_loop->fix_inc;
	pall->ParamsValues +=
		loop_ofs->FixUpdate.FstVal + proc_loop->per_fst * 2;
	pall->ParamsIndices +=
		loop_ofs->FstLoop.FstInd + proc_loop->per_fst;
	ponly->LoopsNum = pio->n_invocations_per_input_tile;
	ponly->LoopsTot.ExpPerSet = proc_loop->n_exp_per_sets;
	ponly->LoopsTot.SetsNum = proc_loop->n_sets;
	ponly->LoopsTot.FixInc = proc_loop->fix_inc;
	ponly->LoopsTot.PerFst = proc_loop->per_fst;
}

/* help structure - save temp process totals. used to set process offsets
 * and task totals
 */
struct vpul_proc_sums {
	struct VPUI_ProcFst	task_proc_ofs;
	struct VPUI_Proc3DNNFst	task_3dnn_proc_ofs;
	__u32			n_procs;
	__u32			n_procs3d;
	__u32			n_HwReadResBl;
};

/* Counts the number of internal ram descriptors - each descriptor potentially maps to
   number of MPRB groups descriptor, but there should be one internal ram descriptor for each
   internal ram */
static __u32 count_proc_int_ram_desc(
	const struct vpul_task *task,
	const struct vpul_vertex *vtx )
{
	__u32 ret_val = 0;
	__u32 i;
	const struct vpul_process_inout *pio = &vtx->proc.io;
	const struct vpul_subchain *sc = fst_vtx_sc_ptr(task, vtx);
	for( i = 0; i< pio->n_inout_types; i++)
	{
		__u32 memmap_index =
			(pio->fixed_map_roi[pio->inout_types[i].roi_index].memmap_idx);
		if(((task->memmap_desc[memmap_index]).mtype == VPUL_MEM_INTERNAL ||
			(task->memmap_desc[memmap_index]).mtype == VPUL_MEM_PRELOAD_PU )){
				ret_val++;

		}
	}


	/* Need to scan also CPU sub chains */
	for(i=0;i<vtx->num_of_subchains;i++,sc++ )
	{
		if(sc->stype == VPUL_SUB_CH_CPU_OP){
			ret_val+=sc->cpu.CpuRamIndices;
		}
	}
	return ret_val;

}
static void set_proc_io_totals(
	const struct vpul_task *task,
	const struct vpul_vertex *vtx,
	struct	VPUI_ProcTotals *ptotals)
{
	const struct vpul_process_inout *pio = &vtx->proc.io;
	struct VPUI_ProcOnlyTotals *ponly = &ptotals->Only;
	struct VPUI_AllTotals *pall = &ptotals->All;
	__u32 i;

	pall->InternalRams = count_proc_int_ram_desc( task, vtx );

	ptotals->Task.PreSetsMemVecs = pio->n_presets_map_desc;
	ptotals->Task.PostSetsMemVecs = pio->n_postsets_map_desc;
	ptotals->Task.InOutTypes = pio->n_inout_types;
	ptotals->Task.Rois = 0;
	ptotals->Task.FixMapRois = pio->n_fixed_map_roi;
	ptotals->Task.MaxDynMapRois = pio->n_dynamic_map_rois;

	for (i = 0; i < pio->n_fixed_map_roi; i++)
		ptotals->Task.Rois += pio->fixed_map_roi[i].use_roi ? 1:0;
	/* TODO: set dynamic (pio->dynamic_map_roi[i]) */


	ptotals->Task.FixPoints = 0;/* TODO: */
	ptotals->Task.SizesOp = pio->n_sizes_op;

	/* sub-chain invoked only once for all input sets */
	ponly->SubChBefInputSets = pio->n_subchain_before_insets;
	ponly->SubChAftInputSets = pio->n_subchain_after_insets;

	ponly->MaxWorkSizeIndices = 0;	/* TODO: Handle this field */

	ponly->PtsMapsSets = pio->pts_maps_sets;
	ponly->PtsMapsEachSet = pio->pts_maps_each_set;
	ptotals->Task.PtMapIndices = pio->pts_maps_sets * pio->pts_maps_each_set;
	ptotals->Task.PtLists = pio->pt_list_num;
	ptotals->Task.RoiFromPt = pio->pt_list_num;
	ptotals->Task.MaxDynPoints = pio->n_dyn_points;
	ptotals->Task.PtRoiSizes = pio->pt_roi_sizes;

}

static void set_3dnn_proc_io_totals(
			const struct vpul_3dnn_process_base *proc_3dnn_base,
			struct VPUI_Proc3DNNTotals *ptotals)
{
	struct VPUI_Proc3DNNOnlyTotals *ponly = &ptotals->Only;
	struct VPUI_AllTotals *pall = &ptotals->All;
	struct VPUI_Proc3DNNAndTaskTotals *task_totals = &ptotals->Task;
	struct VPUI_Proc3DNNEachLayerTotals *each_layer_total = &ponly->EachLayer;
	const struct vpul_3dnn_process_inout *pio = &proc_3dnn_base->io;
	__u32 n_layers = proc_3dnn_base->number_of_layers;
	__u32 i;

	/* 3D NN process doesn't have internal rams definitions */
	pall->InternalRams = 0;

	task_totals->CropInd = pio->n_croppers * n_layers;
	task_totals->ScaleInd = pio->n_scales * n_layers;
	task_totals->InOut3DNN = pio->n_3dnn_inouts * n_layers;
	task_totals->Sizes3DOp = pio->n_sizes_op;

	/* TODO : should be optimized (this is "worst case") */
	task_totals->IoXY3DNNSizes = n_layers * VPUI_XYSIZE_ALL;

	each_layer_total->CropIndices = pio->n_croppers;
	each_layer_total->ScaleIndices = pio->n_scales;
	each_layer_total->InOut3DNN = pio->n_3dnn_inouts;
	each_layer_total->HwParams = 0;
	each_layer_total->ProcParams = 0;
	each_layer_total->DummyAlign = 0;

	ponly->MaxWorkSizeIndices = 0;	/* TODO: Handle this field */
	ponly->DummyAlign = 0;

	/* no parameters replacement per slice group type at this stage */
	for (i = 0; i < VPUI_SLGR_PARS_ALL; i++)
		ponly->SlGrTypeParams[i] = 0;
}


/* set number of sub elements in process + offsets */
static void set_proc_totals(
	const struct vpul_task *task,
	const struct vpul_vertex *vtx,
	struct	VPUI_ProcTotals *ptotals,
	struct	VPUI_ProcParamsOfs *pofs)
{
	const struct vpul_process *proc = &vtx->proc;
	struct VPUI_AllTotals *pall = &ptotals->All;

	pall->ParamsValues = VPUI_PROC_PAR_ALL;
	pofs->HwParamsValues = pall->ParamsValues;



	ptotals->Only.SubChFstLoopOnly = vtx->n_subchain_on_first_iteration;
	ptotals->Only.SubChLastLoopOnly = vtx->n_subchain_on_last_iteration;


	/* sub-chain and pus totals */
	set_proc_sub_chains_totals(task, vtx, ptotals, pofs);

	/* set process descriptor for loop offsets and totals */
	set_proc_loops_totals(&proc->io, ptotals, pofs);

	/* set process input/output descriptor size */
	set_proc_io_totals(task, vtx, ptotals);
}

/* set number of sub elements in process + offsets */
static void set_3dnn_proc_totals(
		const struct vpul_task *task,
		const struct vpul_vertex *vtx,
		struct	VPUI_Proc3DNNTotals *ptotals,
		struct	VPUI_3DNNParamsOfs *pofs,
		const struct vpul_3dnn_process_base *proc_3dnn_base)
{
	struct VPUI_AllTotals *pall = &ptotals->All;

	memset(pofs, 0, sizeof(struct VPUI_3DNNParamsOfs));
	pall->ParamsValues = 0;
	pofs->HwParamsValues = pall->ParamsValues;

	ptotals->Task.Layers3DNN = proc_3dnn_base->number_of_layers;

	/* sub-chain and pus totals */
	set_3dnn_proc_sub_chains_totals(task, vtx, ptotals, pofs);

	/* set process input/output descriptor size */
	set_3dnn_proc_io_totals(proc_3dnn_base, ptotals);
}

static void set_proc_desc_offsets(
	const struct vpul_process_inout *pio,
	struct VPUI_ProcDesc *mbd_proc,
	struct vpul_proc_sums *proc_sums
	)
{
	struct VPUI_ProcFst *t_proc_ofs = &proc_sums->task_proc_ofs;
	struct VPUI_ProcAnd3DNNProcFst *pboth = &t_proc_ofs->Both;
	const struct VPUI_AllTotals *atot = &mbd_proc->Totals.All;

	mbd_proc->Fst = *t_proc_ofs;

	pboth->SubChFstIndOfs.Bl.Block += atot->HwSubCh.Blocks;

	pboth->CpuSubCh += atot->CpuSubCh;

	pboth->SubChFstIndOfs.Bl.Params += atot->ParamsValues;
	pboth->SubChFstIndOfs.Bl.HwConnections += atot->HwSubCh.HwConnections;
	pboth->SubChFstIndOfs.Bl.HwReadResBl += atot->HwSubCh.HwReadResBl;
	pboth->SubChFstIndOfs.Other.HwMprbsGr += atot->HwSubCh.HwMprbsIndGr;
	pboth->SubChFstIndOfs.Other.IntRamInd += atot->HwSubCh.InternalRamsInd;

	pboth->ParametersIndices += atot->ParamsIndices;
	pboth->CpuOps += atot->CpuOps;
	pboth->InternalRams += atot->InternalRams;
	pboth->SubCh += atot->SubCh;

	t_proc_ofs->HwSubCh += mbd_proc->Totals.Task.HwSubCh;
	t_proc_ofs->PreSetsMemVecs += mbd_proc->Totals.Task.PreSetsMemVecs;
	t_proc_ofs->PostSetsMemVecs += mbd_proc->Totals.Task.PostSetsMemVecs;
	t_proc_ofs->InOutTypes += mbd_proc->Totals.Task.InOutTypes;
	t_proc_ofs->SizesOp += mbd_proc->Totals.Task.SizesOp;

	t_proc_ofs->Roi += mbd_proc->Totals.Task.Rois;
	t_proc_ofs->FixMapRoi += mbd_proc->Totals.Task.FixMapRois;
	t_proc_ofs->DynMapRoi += mbd_proc->Totals.Task.MaxDynMapRois;

	t_proc_ofs->PtLists	 += mbd_proc->Totals.Task.PtLists;
	t_proc_ofs->PtRoiSizes	 += mbd_proc->Totals.Task.PtRoiSizes;
	t_proc_ofs->RoiFromPt	 += mbd_proc->Totals.Task.RoiFromPt;
	t_proc_ofs->PtMapIndices += mbd_proc->Totals.Task.PtMapIndices;
	t_proc_ofs->FixPoints	 += mbd_proc->Totals.Task.FixPoints;
	t_proc_ofs->DynPoints	 += mbd_proc->Totals.Task.MaxDynPoints;

	/* TODO: implement when FW code will be ready */
	mbd_proc->InpLoop.LoopSizes.Height = 0;
	mbd_proc->InpLoop.LoopSizes.Width = 0;
	mbd_proc->InpLoop.LoopOfsX = 0;
	mbd_proc->InpLoop.LoopOfsY = 0;

	proc_sums->n_HwReadResBl = atot->HwSubCh.HwReadResBl;

	/* maximal input sets in slot should be minimum 1 */
	mbd_proc->MaxInputSetsInSlot = pio->max_input_sets_slots ?
		pio->max_input_sets_slots : 1;
}

static void set_3dnn_proc_desc_offsets(
		const struct vpul_3dnn_process_inout *pio,
		struct VPUI_Proc3DNNBase *mbd_proc,
		struct vpul_proc_sums *proc_sums)
{
	struct VPUI_Proc3DNNFst *t_proc_ofs = &proc_sums->task_3dnn_proc_ofs;
	struct VPUI_ProcAnd3DNNProcFst *pboth = &t_proc_ofs->Both;
	const struct VPUI_AllTotals *atot = &mbd_proc->Totals.All;
	struct VPUI_Proc3DNNAndTaskTotals *proc_3dnn_and_task_totals = &mbd_proc->Totals.Task;

	mbd_proc->Fst = *t_proc_ofs;

	pboth->SubChFstIndOfs.Bl.Block += atot->HwSubCh.Blocks;

	pboth->CpuSubCh += atot->CpuSubCh;

	pboth->SubChFstIndOfs.Bl.Params += atot->ParamsValues;
	pboth->SubChFstIndOfs.Bl.HwConnections += atot->HwSubCh.HwConnections;
	pboth->SubChFstIndOfs.Bl.HwReadResBl += atot->HwSubCh.HwReadResBl;
	pboth->SubChFstIndOfs.Other.HwMprbsGr += atot->HwSubCh.HwMprbsIndGr;
	pboth->SubChFstIndOfs.Other.IntRamInd += atot->HwSubCh.InternalRamsInd;

	pboth->ParametersIndices += atot->ParamsIndices;
	pboth->CpuOps += atot->CpuOps;
	pboth->InternalRams += atot->InternalRams;
	pboth->SubCh += atot->SubCh;

	t_proc_ofs->Hw3DNNSubCh += proc_3dnn_and_task_totals->Hw3DNNSubCh;
	t_proc_ofs->Hw3DNNSlGrTypeSubCh += proc_3dnn_and_task_totals->Hw3DNNSlGrTypeSubCh;
	t_proc_ofs->InOut3DNN += proc_3dnn_and_task_totals->InOut3DNN;
	t_proc_ofs->IoXY3DNNSizes += proc_3dnn_and_task_totals->IoXY3DNNSizes;
	t_proc_ofs->Sizes3DOp += proc_3dnn_and_task_totals->Sizes3DOp;
	t_proc_ofs->CropInd += proc_3dnn_and_task_totals->CropInd;
	t_proc_ofs->ScaleInd += proc_3dnn_and_task_totals->ScaleInd;
	t_proc_ofs->Layers3DNN += proc_3dnn_and_task_totals->Layers3DNN;

	proc_sums->n_HwReadResBl += atot->HwSubCh.HwReadResBl;

}

static void set_proc_desc_for_map_2_list
(
	const struct vpul_task *task,
	struct VPUI_ProcDesc *mbd_proc,
	const struct vpul_process_inout *proc_io
)
{
	__u32  i, j, k;
	struct vpul_vertex *vtx;
	struct vpul_subchain *subchain;
	struct vpul_pu *pu;

	/*  Need to update parameters */

	mbd_proc->OutList.RecordBytes = 0;
	mbd_proc->OutList.MaxRecords = 0;
	mbd_proc->OutList.InOutTypeIndexP1 = 0;
	mbd_proc->OutList.ExtAdrInd = 0;
	mbd_proc->OutList.DummyAlign = 0;

	vtx = fst_vtx_ptr(task);
	for (k = 0; k < task->t_num_of_vertices; vtx++, k++) {
		subchain = fst_vtx_sc_ptr(task, vtx);
		for (j = 0; j < vtx->num_of_subchains; j++, subchain++) {
			if (subchain->stype != VPUL_SUB_CH_CPU_OP){
				pu = fst_sc_pu_ptr(task, subchain);
				for (i = 0; i < subchain->num_of_pus; i++, pu++) {
					if (pu->op_type == VPUL_OP_MAP_2_LST) {
						/* in order to connect properly the */
						/* output DMA to M2LI               */
						mbd_proc->OutList.RecordBytes = 4;
						if(pu->params.map2list.value_in != 0){
							/*if we output value 2 more bytes need for record*/
							mbd_proc->OutList.RecordBytes+=2;
							if(pu->params.map2list.bits_in==2){
								/*if we use 32 bits value we need 4 bytes extra for value*/
								mbd_proc->OutList.RecordBytes+=2;
							}
						}

						mbd_proc->OutList.MaxRecords =
							pu->params.map2list.num_of_point;
						mbd_proc->OutList.InOutTypeIndexP1 =
							pu->params.map2list.inout_indx+1;

						if(((task->
							memmap_desc[pu->params.map2list.inout_indx]).mtype)==
							VPUL_MEM_EXTERNAL){
							mbd_proc->OutList.ExtAdrInd =
								task->memmap_desc
								[pu->params.map2list.inout_indx].index;

						}
						else{
							mbd_proc->OutList.ExtAdrInd = 0;
						}
						mbd_proc->OutList.DummyAlign = 0;
						break;
					}
				}
			}
		}
	}
}


/* set process descriptor */
static __s32 tc_set_process_desc(
	struct internal_context* intrnl_ctx,
	const struct vpul_task *task,
	struct vpul_proc_sums *proc_sums,
	struct VPUI_ProcDesc *mbd_proc
	)
{
	__u32 input_loop = 0; /* TODO: add support for input loops */
	__u32 cpu_only;
	__u32 is_tile_div;
	__u32 input_set_div = 1;
	__u32 i;
	const struct vpul_iotypes_desc *ioty;

	struct vpul_vertex *vtx = vtx_ptr(task,(intrnl_ctx->tds_itrtr_stat.current_vtx_id));
	set_proc_totals(task, vtx, &mbd_proc->Totals, &mbd_proc->ParsOfs);

	cpu_only = (mbd_proc->Totals.All.CpuSubCh == mbd_proc->Totals.All.SubCh);
	BUG_ON_NOT_IMPLEMENTED_FUNCT(vtx->proc.io.n_tiles > 0);
	/* is_tile_div = vtx->proc.io.n_tiles > 0 ? 1 : 0; */
	is_tile_div = 0;
	mbd_proc->ProcBits =
		(input_loop << PROC_IS_INPUT_LOOP_LSB) |
		(is_tile_div << PROC_IS_TILE_DIV_LSB) |
		(cpu_only << PROC_IS_CPU_ONLY_LSB) |
		(task->priority << PROC_PRIORITY_LSB);

	ioty = vtx->proc.io.inout_types;
	for (i = 0; i < vtx->proc.io.n_inout_types; i++, ioty++) {
		BUG_ON_NOT_IMPLEMENTED_FUNCT(
			(ioty->n_insets_per_dynamic_roi != 0 &&
			(input_set_div %
			ioty->n_insets_per_dynamic_roi) != 0));
		/* if (ioty->n_insets_per_dynamic_roi != 0 &&
		 * (input_set_div % ioty->n_insets_per_dynamic_roi) != 0)
		 * input_set_div *= ioty->n_insets_per_dynamic_roi;
		 */
	}
	mbd_proc->InputSetsDivider = input_set_div;

	set_proc_desc_for_map_2_list(task, mbd_proc, &(vtx->proc.io));

	/* process offsets to it's descriptors */
	set_proc_desc_offsets(&vtx->proc.io, mbd_proc, proc_sums);

	return VPU_STATUS_SUCCESS;
}

static void tc_set_3dnn_process_base_desc(const struct vpul_task *task,
					const struct vpul_3dnn_process_base *proc_3dnn_base,
					__u32 proc_base_3dnn_idx,
					struct vpul_proc_sums *proc_sums,
					struct VPUI_Proc3DNNBase *mbd_proc_3dnn_base)
{
	const struct vpul_vertex *vtx;

	vtx = vertex_referencing_this_3dnn_proc_base(task, proc_base_3dnn_idx);
	if (vtx) {
		/* vertex found (if not : do nothing, exit right away) */
		set_3dnn_proc_totals(task, vtx, &mbd_proc_3dnn_base->Totals,
					&mbd_proc_3dnn_base->Params3DNNOfs,
					proc_3dnn_base);

		/* process offsets to it's descriptors */
		set_3dnn_proc_desc_offsets(&proc_3dnn_base->io,
					mbd_proc_3dnn_base, proc_sums);
	}
}

/* calculate number of task loop parameters and indices */
static void set_tot_loops(
	const struct vpul_task *task,
	__u16 *loop_total,
	__u32 *loops_indices
	)
{
	const struct vpul_vertex *vtx = fst_vtx_ptr(task);
	__u32 i;

	*loop_total = 0;
	*loops_indices = 0;
	for (i = 0; i < task->t_num_of_vertices; i++, vtx++) {
		if (vtx->loop.type == VPU_LOOP_END) {
			(*loop_total) += 0;
			(*loops_indices) += 0;
		}
	}
}

/* calculate number of task loop parameters and indices */
static void set_task_total_params_and_indices(
	const struct vpul_task *task,
	struct VPUI_TaskCreate *mb_tc,
	__u32 block_params,
	__u32 indices_params
	)
{
	struct VPUI_AllTotals *allt = &mb_tc->Totals.AllTotals;
	struct VPUI_TaskParamsValuesOfs	*valfs = &mb_tc->ParamsParts.ValuesOfs;
	struct VPUI_TaskParamsIndicesOfs *paridx = &mb_tc->ParamsParts.Indices;
	__u32 loop_indices;

	/* calculate number of task loop parameters and indices */
	set_tot_loops(task, &mb_tc->ParamsParts.TaskLoopsTotal, &loop_indices);

	mb_tc->ParamsParts.TaskAllocate = 0;/* TODO: */

	/* get number of parameters updated on each task invocation */
	mb_tc->ParamsParts.TaskInvoke = calc_mb_create_invoke_param_size_from_task(task);

	/* first parameter is for task edges disable mask */
	allt->ParamsValues = VPUI_TASKPAR_ALL;
	valfs->Procs = allt->ParamsValues;
	allt->ParamsValues += block_params;
	valfs->TaskLoops = allt->ParamsValues;
	/* TODO: add 3DNN processes parameters */
	allt->ParamsValues += mb_tc->ParamsParts.TaskLoopsTotal;
	valfs->ImageDesc = allt->ParamsValues;
	allt->ParamsValues += mb_tc->Totals.ProcTotals.InOutTypes *VPUI_IMDESC_TOT_PARAMS;//task->n_memmap_desc *VPUI_IMDESC_TOT_PARAMS;
	valfs->DynFiltCoeffs = allt->ParamsValues;
	allt->ParamsValues += mb_tc->Totals.FiltDynCoeffs;

	/* calc ofsset from ParamsParts.Indices and add sizes to totals */
	/* paridx: mb_tc->ParamsParts.Indices - ofst of idxs location upon idxs vector basline. */
	/* allt:   mb_tc->Totals.AllTotals - sum of sizes for various entities (example: indices)*/
	allt->ParamsIndices = 0;
	paridx->Procs        = allt->ParamsIndices;
	allt->ParamsIndices += indices_params;
	paridx->TaskLoops    = allt->ParamsIndices;
	allt->ParamsIndices += loop_indices;
	paridx->TaskAllocate = allt->ParamsIndices;
	allt->ParamsIndices += mb_tc->ParamsParts.TaskAllocate;
	paridx->TaskInvoke   = allt->ParamsIndices;
	allt->ParamsIndices += mb_tc->ParamsParts.TaskInvoke;
}

static void calc_coef_each_filt_and_total_for_vertex
(
	const struct vpul_task *task,
	const struct vpul_vertex *vtx,
	__u32 num_static_coeff,
	__u16 *sep_filt_coeff_sets,
	__u16 *gen_filt_coeff_sets,
	__u16 *filt_static_coeffs,
	__u16 *filt_dyn_coeffs
)
{
	__u32 i, j;
	const struct vpul_pu *pu;
	__u32 sep_filter_coef_cntr = 0;
	__u32 gen_filter_coef_cntr = 0;

	const struct vpul_subchain *subchain = fst_vtx_sc_ptr(task, vtx);

	for (j = 0; j < vtx->num_of_subchains; j++, subchain++) {
		if (subchain->stype != VPUL_SUB_CH_CPU_OP){
			pu = fst_sc_pu_ptr(task, subchain);
			for (i = 0; i < subchain->num_of_pus; i++, pu++) {
				if (VPUL_OP_SEP_FLT == pu->op_type)
				{
					if (1 == pu->params.slf.vertical_filter_coeff.is_dynamic)
					{
						*filt_dyn_coeffs +=
							(pu->params.slf.vertical_filter_coeff.vsize * 2);
					}

					sep_filter_coef_cntr++;
				}

				if (VPUL_OP_GEN_FLT == pu->op_type)
				{
					if (1 == pu->params.glf.filter_coeff.is_dynamic){
						*filt_dyn_coeffs +=
							pu->params.glf.filter_coeff.vsize;
					}

					gen_filter_coef_cntr++;
				}
			}
		}
	}

	/* copy filter params of each static filter */
	*sep_filt_coeff_sets += sep_filter_coef_cntr;

	/* copy filter params of each static filter */
	*gen_filt_coeff_sets += gen_filter_coef_cntr;

	if ((gen_filter_coef_cntr > 0) ||
		(sep_filter_coef_cntr > 0))
	{
		*filt_static_coeffs += num_static_coeff;
	}
}

static __u8 calc_number_of_task_loops(const struct vpul_task *task){

	struct vpul_vertex *	vtx = vtx_ptr(task, 0);
	__u8			n_loops = 0;
	__u32 i;
	for(i=0;i<task->t_num_of_vertices;i++,vtx++){
		if(vtx->loop.type == VPU_LOOP_END){
			n_loops++;
		}
	}
	return n_loops;
}


static const __u32 bits_on[16]=
{/*	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F */
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};

static __u32 count_bits(__u32 value){

	__u32 i=0;
	__u32 bits_sum = 0;
	for(;i<8;i++){
		bits_sum += bits_on[value&0xF];
		value = value >> 4;
	}
	return bits_sum;
}
static __u16  calc_max_num_of_mprbg_needed_for_internal_rams(const struct vpul_task *task){

	__u32 index = 0;
	__u16 ret_val = 0;
	for(;index<task->n_memmap_desc;index++){
		if( task->memmap_desc[index].mtype==VPUL_MEM_INTERNAL){
			//Currently internal ram description can only work with one MPRB
			ret_val++;
		}
		else if(task->memmap_desc[index].mtype==VPUL_MEM_PRELOAD_PU){
				ret_val+=count_bits(task->memmap_desc[index].pu_index.ports_bit_map);
		}
	}

	return ret_val;

}
static void calculate_coef_each_filt_and_total(
		const struct vpul_task *task,
		__u16 *sep_filt_coeff_sets,
		__u16 *gen_filt_coeff_sets,
		__u16 *filt_static_coeffs,
		__u16 *filt_dyn_coeffs)
{
	__u32  k;
	const struct vpul_vertex *vtx = fst_vtx_ptr(task);
	const struct vpul_3dnn_process_base *proc_3dnn_base = fst_3dnn_process_base_ptr(task);

	*sep_filt_coeff_sets  = 0;
	*gen_filt_coeff_sets  = 0;
	*filt_static_coeffs	  = 0;
	*filt_dyn_coeffs	  = 0;

	for (k = 0; k < task->t_num_of_vertices; vtx++, k++) {
			if (vtx->vtype == VPUL_VERTEXT_PROC){
			calc_coef_each_filt_and_total_for_vertex(task,
								vtx,
								vtx->proc.io.n_static_coff,
								sep_filt_coeff_sets,
								gen_filt_coeff_sets,
								filt_static_coeffs,
								filt_dyn_coeffs);
		}
	}
	for (k = 0; k < task->t_num_of_3dnn_process_bases; k++, proc_3dnn_base++) {
		vtx = vertex_referencing_this_3dnn_proc_base(task, k);
		if (vtx)
			calc_coef_each_filt_and_total_for_vertex(task,
								vtx,
								proc_3dnn_base->io.n_static_coff,
								sep_filt_coeff_sets,
								gen_filt_coeff_sets,
								filt_static_coeffs,
								filt_dyn_coeffs);
	}
}

static __u32 get_total_hist_params( const struct vpul_task *task )
{
	__u32 n_hist_pu=0;
	__u32 count = 0;

	struct vpul_pu * pu = fst_pu_ptr(task);

	for(;count<task->t_num_of_pus;count++,pu++){
		if(pu->op_type==VPUL_OP_HIST){
			n_hist_pu++;
		}
	}

	return n_hist_pu;
}

static __u32 get_total_fix_thr_params( const struct vpul_task *task )
{
	__u32 n_fix_thr = 0;
	__u32 count = 0;

	struct vpul_pu * pu = fst_pu_ptr(task);

	for(; count<task->t_num_of_pus; count++, pu++){
		if( (pu->op_type == VPUL_OP_FAST_DEPTH) ||
			(pu->op_type == VPUL_OP_IN_PAINT)   ||
			(pu->op_type == VPUL_OP_DISPEQ)){
			n_fix_thr++;

			break;
		}
	}

	return n_fix_thr;
}

static void calculate_tot_ops(const struct vpul_task *task,
				struct VPUI_TaskTotals *totals)
{
	const struct vpul_3dnn_layer *layer_3dnn;
	const struct vpul_vertex *vtx = fst_vtx_ptr(task);
	const struct vpul_3dnn_process_base *proc_3dnn_base = fst_3dnn_process_base_ptr(task);
	__u32 number_of_layers;
	__u32  i, k;
	__u32  crop_ops_cntr = 0;
	__u32  fix_ops_cntr = 0;
	__u32  scale_ops_cntr = 0;
	__u32  relu_cntr = 0;
	__u32  cnn_crop_cntr = 0;
	__u32  cnn_scaler_cntr = 0;

	for (k = 0; k < task->t_num_of_vertices; vtx++, k++){
		if (vtx->vtype == VPUL_VERTEXT_PROC) {
			for (i = 0; i < vtx->proc.io.n_sizes_op; i++)
			{
				if (VPUL_SIZEOP_FORCE_CROP == vtx->proc.io.sizes[i].type)
					crop_ops_cntr++;
				else if (VPUL_SIZEOP_FIX == vtx->proc.io.sizes[i].type)
					fix_ops_cntr++;
				else if (VPUL_SIZEOP_SCALE == vtx->proc.io.sizes[i].type)
					scale_ops_cntr++;
			}
		}
	}
	for (k = 0; k < task->t_num_of_3dnn_process_bases; proc_3dnn_base++, k++) {
		number_of_layers = proc_3dnn_base->number_of_layers;
		scale_ops_cntr += (proc_3dnn_base->io.n_scales * number_of_layers);
		crop_ops_cntr += (proc_3dnn_base->io.n_croppers * number_of_layers);
		for (i = 0; i < number_of_layers; i++) {
			layer_3dnn = &proc_3dnn_base->layers[i];
			cnn_scaler_cntr += layer_3dnn->n_scale_factors;
			if (layer_3dnn->cnn_descr.xy_crop == VPU_CNN_XY_CROP_EXPLICIT)
				cnn_crop_cntr++;
			if (layer_3dnn->cnn_descr.is_relu_enabled)
				relu_cntr++;
		}
	}

	totals->CropOps = crop_ops_cntr;
	totals->FixOps = fix_ops_cntr;
	totals->ScaleOps = scale_ops_cntr;
	totals->ReluDesc = relu_cntr;
	totals->CnnCropDesc = cnn_crop_cntr;
	if (task->t_num_of_3dnn_process_bases > 0)
		totals->CNNScalers = cnn_scaler_cntr + 1;  /* include default scaler */
	else
		totals->CNNScalers = 0;
}

/* set totals number of elements on task level */
static void set_task_totals(
	const struct vpul_task *task,
	const struct vpul_proc_sums *proc_sums,
	struct VPUI_TaskTotals *tot,
	__u32  num_invoke_extenal_mem)
{
	struct VPUI_AllHwSubChTotals *hwsct = &tot->AllTotals.HwSubCh;
	const struct VPUI_ProcFst *tp_ofs = &proc_sums->task_proc_ofs;
	const struct VPUI_Proc3DNNFst *tp_3dnn_ofs = &proc_sums->task_3dnn_proc_ofs;

	hwsct->Blocks = tp_ofs->Both.SubChFstIndOfs.Bl.Block +
				tp_3dnn_ofs->Both.SubChFstIndOfs.Bl.Block;
	hwsct->HwConnections = tp_ofs->Both.SubChFstIndOfs.Bl.HwConnections +
				tp_3dnn_ofs->Both.SubChFstIndOfs.Bl.HwConnections;
	hwsct->HwMprbsIndGr = tp_ofs->Both.SubChFstIndOfs.Other.HwMprbsGr +
				tp_3dnn_ofs->Both.SubChFstIndOfs.Other.HwMprbsGr;
	hwsct->InternalRamsInd = tp_ofs->Both.SubChFstIndOfs.Other.IntRamInd +
				tp_3dnn_ofs->Both.SubChFstIndOfs.Other.IntRamInd;
	hwsct->HwReadResBl = proc_sums->n_HwReadResBl;
	tot->AllTotals.CpuOps = tp_ofs->Both.CpuOps + tp_3dnn_ofs->Both.CpuOps;
	tot->AllTotals.InternalRams = tp_ofs->Both.InternalRams +
				tp_3dnn_ofs->Both.InternalRams;
	tot->AllTotals.SubCh = tp_ofs->Both.SubCh + tp_3dnn_ofs->Both.SubCh;
	tot->AllTotals.CpuSubCh = tp_ofs->Both.CpuSubCh + tp_3dnn_ofs->Both.CpuSubCh;
	tot->ProcTotals.HwSubCh = tp_ofs->HwSubCh;
	tot->ProcTotals.PreSetsMemVecs = tp_ofs->PreSetsMemVecs;
	tot->ProcTotals.PostSetsMemVecs = tp_ofs->PostSetsMemVecs;
	tot->ProcTotals.InOutTypes = tp_ofs->InOutTypes;
	tot->ProcTotals.SizesOp = tp_ofs->SizesOp;

	tot->ProcTotals.Rois = tp_ofs->Roi;
	tot->ProcTotals.FixMapRois = tp_ofs->FixMapRoi;
	tot->ProcTotals.MaxDynMapRois = tp_ofs->DynMapRoi;
	tot->ProcTotals.PtLists = tp_ofs->PtLists;
	tot->ProcTotals.PtRoiSizes = tp_ofs->PtRoiSizes;
	tot->ProcTotals.RoiFromPt = tp_ofs->RoiFromPt;
	tot->ProcTotals.PtMapIndices = tp_ofs->PtMapIndices;
	tot->ProcTotals.FixPoints = tp_ofs->FixPoints;
	tot->ProcTotals.MaxDynPoints = tp_ofs->DynPoints;

	tot->Proc3DNNTotals.Hw3DNNSlGrTypeSubCh = tp_3dnn_ofs->Hw3DNNSlGrTypeSubCh;
	tot->Proc3DNNTotals.Hw3DNNSubCh = tp_3dnn_ofs->Hw3DNNSubCh;
	tot->Proc3DNNTotals.IoXY3DNNSizes = tp_3dnn_ofs->IoXY3DNNSizes;
	tot->Proc3DNNTotals.InOut3DNN = tp_3dnn_ofs->InOut3DNN;
	tot->Proc3DNNTotals.CropInd = tp_3dnn_ofs->CropInd;
	tot->Proc3DNNTotals.ScaleInd = tp_3dnn_ofs->ScaleInd;
	tot->Proc3DNNTotals.Sizes3DOp = tp_3dnn_ofs->Sizes3DOp;
	tot->Proc3DNNTotals.Layers3DNN = tp_3dnn_ofs->Layers3DNN;

	tot->OutputOffsets = 0; /* TODO: */
	tot->OutputTotal16BSize = 0; /* TODO: */
	tot->HistParams = get_total_hist_params( task);
	calculate_coef_each_filt_and_total(task,
		&tot->SepFiltCoeffSets,
		&tot->GenFiltCoeffSets,
		&tot->FiltStaticCoeffs,
		&tot->FiltDynCoeffs);

	tot->RamsMprbsIndGr = calc_max_num_of_mprbg_needed_for_internal_rams(task);
	tot->DisparityFixedThr = get_total_fix_thr_params(task);
	tot->DepthDynThr = 0;			/* Currently Dynamic params adj to pu params */
	tot->InPaintDynThr = 0;			/* Currently Dynamic params adj to pu params */
	tot->Tiles = 0;				/*TODO*/
	calculate_tot_ops(task, tot);
	tot->Procs = proc_sums->n_procs;
	tot->Procs3DNNBase = task->t_num_of_3dnn_process_bases;
	tot->Procs3DNN = proc_sums->n_procs3d;
	tot->ExternalBaseAdrOfs = task->n_external_mem_addresses;
	tot->ImageDesc = task->n_memmap_desc;
	tot->MapDesc = task->n_memmap_desc;
	tot->ExtSlotOfs = 0;			/*TODO*/
	tot->SlotMem = 0;			/*TODO*/
	tot->EachInvokeExtMemAdr = num_invoke_extenal_mem;
	tot->Vertex = task->t_num_of_vertices;
	calc_num_of_edges(task, &tot->Edges);
	tot->InEdgesGroups = 0;			/* Not used by translator */
	tot->TaskLoops = calc_number_of_task_loops(task);
}

/* set Command section on task create, and processes descriptors */
static __s32 set_task_create_command(
	struct internal_context* intrnl_ctx,
	const struct vpul_task *task,
	const struct vpu_mark_for_invoke_ext_mem_vec_ds *invoke_mem_mark,
	struct VPUI_TaskCreate *mb_tc,
	__u8 *mb_cp,
	__u32 *p_mb_cp_size
	)
{
	struct VPUI_ProcDesc *mbd_proc;
	struct VPUI_Proc3DNNBase *mbd_proc_3dnn_base;
	const struct vpul_vertex *vtx = fst_vtx_ptr(task);
	const struct vpul_3dnn_process_base *proc_3dnn_base;
	__u32 i;
	struct vpul_proc_sums proc_sums_st;
	__u32 num_invoke_extenal_mem = 0;
	__u32 predefined_size;
	__u32 offset = 0;
	__u32 nb_3dnn_proc_base = task->t_num_of_3dnn_process_bases;

	memset((void *)&proc_sums_st, 0, sizeof(struct vpul_proc_sums));

	mb_tc->Header.Priority = task->priority;
	mb_tc->Header.IsFixedHw = 0x1; /* Currently we support only fixed HW*/

	/* assuming first descriptor is proc, without any offset */
	mbd_proc = (struct VPUI_ProcDesc *)(mb_cp + offset);
	mb_tc->DataOfs.Procs = offset;


	intrnl_ctx->required_output_mem_allc_16B = 0;

	/* set processes */
	for (i = 0; i < task->t_num_of_vertices; i++, vtx++) {
		intrnl_ctx->tds_itrtr_stat.current_vtx_id = i;
		switch (vtx->vtype) {
		case VPUL_VERTEXT_PROC:
			tc_set_process_desc(intrnl_ctx, task, &proc_sums_st, mbd_proc++);
			proc_sums_st.n_procs++;

			break;
		case VPUL_VERTEXT_3DNN_PROC:
			proc_sums_st.n_procs3d++;
			break;
		case VPUL_VERTEXT_HOST_REP:
			/* TODO: handle host reports */
			break;
		default:
			break;
		}
	}

	offset += m2size(sizeof(struct VPUI_ProcDesc) * proc_sums_st.n_procs);

	mb_tc->DataOfs.Procs3DNNBase = offset;

	mbd_proc_3dnn_base = (struct VPUI_Proc3DNNBase *)(mb_cp + offset);
	proc_3dnn_base = fst_3dnn_process_base_ptr(task);
	for (i = 0; i < nb_3dnn_proc_base; i++, proc_3dnn_base++) {
		tc_set_3dnn_process_base_desc(task, proc_3dnn_base, i,
					&proc_sums_st, mbd_proc_3dnn_base++);
		/* proc_sums_st.n_internal_rams += proc_3dnn_base->io.n_internal_rams; */
	}

	offset += m2size(sizeof(struct VPUI_Proc3DNNBase) * nb_3dnn_proc_base);

	if (invoke_mem_mark != NULL)
		num_invoke_extenal_mem = invoke_mem_mark->num_of_buffers;

	/* set totals number of elements on task level */
	set_task_totals(task,
		&proc_sums_st,
		&mb_tc->Totals,
		num_invoke_extenal_mem);


	/* calculate tasks total amount of parameters and indices */
	set_task_total_params_and_indices(task, mb_tc,
		proc_sums_st.task_proc_ofs.Both.SubChFstIndOfs.Bl.Params +
			proc_sums_st.task_3dnn_proc_ofs.Both.SubChFstIndOfs.Bl.Params,
		proc_sums_st.task_proc_ofs.Both.ParametersIndices +
			proc_sums_st.task_3dnn_proc_ofs.Both.ParametersIndices);

	/* calculate offsets of Mbox descriptors */
	*p_mb_cp_size = mb_desc_vectors_offs(&mb_tc->Totals, &mb_tc->DataOfs, offset);

	/* remove this test if calc_mb_create_param removed */
	predefined_size = calc_mb_create_param_size_from_task(task);
	BUG_ON(*p_mb_cp_size > predefined_size);

	/* calculate total Mbox message size and clear command parameters
	 * Size was already checked earlier - if we get to here with wrong size
	 * is a bug
	 */
	mb_tc->EntireDateSize = sizeof(struct VPUI_CmndHdr) +
		sizeof(struct VPUI_TaskCreate) + *p_mb_cp_size;

	return VPU_STATUS_SUCCESS;
};

/* save offsets to parameter's list for "update parameters" functions */
void save_internal_info(
	struct vpul_task *task,
	const struct VPUI_TaskCreate *mb_tc,
	const __u8 *mb_cp
	)
{
	/* process level info */
	struct VPUI_ProcDesc *mbd_proc = (struct VPUI_ProcDesc *)mb_cp;
	struct vpul_vertex *vtx = fst_vtx_ptr(task);
	struct vpul_alloc_read_only *roaloc = &task->task_ro.alloc;
	__u32 i;

	for (i = 0; i < task->t_num_of_vertices; i++, vtx++) {
		if (vtx->vtype == VPUL_VERTEXT_PROC) {
			vtx->proc.proc_ro.n_proc_hw_params =
				mbd_proc->ParsOfs.HwParamsValues;
			vtx->proc.proc_ro.n_proc_cpu_para =
				mbd_proc->ParsOfs.CpuParamsValues;
			vtx->proc.proc_ro.n_proc_loop_para =
				mbd_proc->ParsOfs.LoopParamsValues;
		}
	}

	/* task level info*/
	task->task_ro.n_invoke_params = mb_tc->ParamsParts.TaskInvoke;
	task->task_ro.n_invoke_extmem_addr = mb_tc->Totals.EachInvokeExtMemAdr;

	roaloc->n_params = mb_tc->ParamsParts.TaskAllocate;
	roaloc->n_ext_base_addrs = mb_tc->Totals.ExternalBaseAdrOfs;
	roaloc->n_blocks = mb_tc->Totals.AllTotals.HwSubCh.Blocks;
	roaloc->n_hwmprbs = mb_tc->Totals.AllTotals.HwSubCh.HwMprbsIndGr;
	roaloc->n_internal_rams = mb_tc->Totals.AllTotals.InternalRams;
	roaloc->n_rammprbs = mb_tc->Totals.RamsMprbsIndGr;
}



static __u32 calc_mb_task_allocate_msg_size(struct internal_context * internal_context,
	struct vpul_task *task )
{
	__u32 psize = 0;
	const struct vpul_alloc_read_only *aro = &task->task_ro.alloc;

	psize += m2size(aro->n_ext_base_addrs * sizeof(struct VPUI_32bMem));
	psize += m2size(aro->n_params * sizeof(__u16));
	psize += m2size(aro->n_blocks * sizeof(__u8));
	psize += m2size(aro->n_hwmprbs * sizeof(struct VPUI_MprbGr));
	psize += m2size(aro->n_rammprbs * sizeof(struct VPUI_MprbGr));
	psize += m2size(aro->n_internal_rams * sizeof(__u8));

	return  sizeof(struct VPUM_HostCmndHdr) +
		sizeof(struct VPUI_CmndHdr) +
		sizeof(struct VPUI_TaskAlloc) +
		psize;
}

static void set_slots(struct VPUI_HwSlotsOfs *slot_ofs, __u32 *pofs,
	__u32 n_elements, __u32 elem_size)
{
	slot_ofs->IsDefaultUsed = 1;
	slot_ofs->DummyAlign = 0;
	slot_ofs->DefaultOfs = *pofs;
	*pofs += m2size(n_elements * elem_size);
	slot_ofs->TotalEachSlot = 0;
	slot_ofs->EachSlotIndicesOfs = *pofs;
	slot_ofs->EachSlotValuesOfs = *pofs;
}

static __u32 ta_set_offsets(
	const struct vpul_task *task,
	struct VPUI_TaskAlloc *mb_ta
	)
{
	const struct vpul_alloc_read_only *aro = &task->task_ro.alloc;
	__u32 ofs = 0;
	struct	VPUI_TaskAllocOfs *ta_ofs = &mb_ta->DataOfs;

	ta_ofs->ExternalMemAdrOfs = ofs;
	ofs += m2size(aro->n_ext_base_addrs * sizeof(struct VPUI_32bMem));
	ta_ofs->ParamsValuesOfs = ofs;
	ofs += m2size(aro->n_params * sizeof(__u16));
	set_slots(&ta_ofs->BlInstances, &ofs,
		aro->n_blocks, sizeof(__u8));
	set_slots(&ta_ofs->HwMprbsGr, &ofs, aro->n_hwmprbs,
		sizeof(struct VPUI_MprbGr));
	set_slots(&ta_ofs->RamsMprbsGr, &ofs,
		aro->n_rammprbs, sizeof(struct VPUI_MprbGr));
	set_slots(&ta_ofs->IntRamFstVirtUnit, &ofs,
		aro->n_internal_rams, sizeof(__u8));
	return ofs;
}

static __s32 ta_set_command(
	const struct vpul_task *task,
	__u32	n_slots,
	struct VPUI_TaskAlloc *mb_ta
		)
{
	__u32 ta_param_size;

	if (n_slots == 0) {
		VPU_DEBG("numberr of slots for task allocation must be>0.\n");
		return VPU_STATUS_BAD_PARAMS;
	}

	mb_ta->SlotsNum = n_slots;
	mb_ta->DummyAlign = 0;
	ta_param_size = ta_set_offsets(task, mb_ta);
	mb_ta->EntireDateSize = sizeof(struct VPUI_CmndHdr) +
		sizeof(struct VPUI_TaskAlloc) + ta_param_size;

	/* TODO: remove this when cleaning padded bytes */
	memset((void *)(mb_ta + 1), 0, ta_param_size);

	return VPU_STATUS_SUCCESS;
}

struct ptr_allocate_str {
	struct VPUI_32bMem *mb_ext_mem_addr;
	__u16 *params;
	__u8 *bl_inst;
	struct VPUI_MprbGr *hw_mprbs;
	struct VPUI_MprbGr *ram_mprbs;
	__u8 *vir_unit;
};


/* internal memories virtual units */
static void ta_set_internal_mem(
	struct ptr_allocate_str *palloc,
		__u8 * first_free_internal_unit,
		__u8 n_vir_unit
	)
{
	*(palloc->vir_unit) = *first_free_internal_unit;
	(*first_free_internal_unit) += n_vir_unit;
	palloc->vir_unit++;
}


/* external memories allocation */
static void ta_set_ex_mem(
	__u32 addr,
	struct ptr_allocate_str *palloc
	)
{
	palloc->mb_ext_mem_addr->LsWord = addr & 0xFFFF;
	palloc->mb_ext_mem_addr->MsWord = addr >> 16;
	palloc->mb_ext_mem_addr++;
}

static void update_mprb_groups_for_pu(struct internal_context * internal_context,
					__u32 proc_idx,
					__u32 sc_idx,
					__u32 pu_idx,
					const struct vpul_pu *pu,
					struct ptr_allocate_str *palloc)
{
	__u32 jj;
	__u32 pu_num_mprb_groups = pu_get_num_mprb_groups_and_fill_groups(pu, palloc->hw_mprbs);

	if (is_preload_needed(pu->instance))
	{
		internal_context->mprb_pu_table[internal_context->n_entries].mprb_group_num
			= palloc->hw_mprbs->MprbsNum;

		/*assuming:preloading PU's has at most MAXNUM_OF_PORTS_FOR_PRELOADING_PU ports.*/
		for (jj=0;jj<MAXNUM_OF_PORTS_FOR_PRELOADING_PU;jj++)
			internal_context->mprb_pu_table[internal_context->n_entries].preloaded_mprb[jj] =
			pu->mprbs[jj];

		internal_context->mprb_pu_table[internal_context->n_entries].proc_idx
			= proc_idx;
		internal_context->mprb_pu_table[internal_context->n_entries].sc_idx
			= sc_idx;
		internal_context->mprb_pu_table[internal_context->n_entries].pu_idx
			= pu_idx;
		internal_context->n_entries++;
	}

	palloc->hw_mprbs += pu_num_mprb_groups;




}

static __s32 ta_check_update_subchains_pus
(
struct internal_context * internal_context,
	__u8 proc_idx,
	__u8 sc_idx,
	const struct vpul_task *task,
	const struct vpul_subchain *sc,
	struct ptr_allocate_str *palloc
)
{
	const struct vpul_pu *first_pu = fst_sc_pu_ptr(task, sc);
	const struct vpul_pu *pu = first_pu;
	__u32 i;

	BUG_ON(task == NULL);

	for (i = 0; i < sc->num_of_pus; i++, pu++) {
		*palloc->bl_inst++ = pu_get_instance_idx(pu->instance);
		update_mprb_groups_for_pu(internal_context,
			proc_idx,
			sc_idx,
			i,
			pu,
			palloc);

	}



	return VPU_STATUS_SUCCESS;
}

/* HW blocks instances, HW connections instances, MPRBs for block allocation */
static __s32 ta_set_proc_pus(struct internal_context * internal_context,
				__u8 proc_idx, const struct vpul_task *task,
				const struct vpul_vertex *vtx, struct ptr_allocate_str *palloc)
{
	const struct vpul_subchain *sc = fst_vtx_sc_ptr(task, vtx);
	__u32 i;
	__s32 stat = VPU_STATUS_SUCCESS;

	for (i = 0; i < vtx->num_of_subchains; i++, sc++) {
		if (sc->stype != VPUL_SUB_CH_CPU_OP) {
			stat = ta_check_update_subchains_pus(internal_context,
				proc_idx,
				i,
				task,
				sc,
				palloc);
			if (VPU_STATUS_IS_FAILURE(stat))
				return stat;
		}
	}
	return stat;
}

/* HW blocks instances, HW connections instances, MPRBs for block allocation */
static __s32 ta_set_3dnn_proc_pus(struct internal_context * internal_context,
				__u8 proc_idx, const struct vpul_task *task,
				const struct vpul_vertex *vtx, struct ptr_allocate_str *palloc)
{
	const struct vpul_subchain *first_sc = fst_vtx_sc_ptr(task, vtx);
	const struct vpul_subchain *sc = first_sc;
	const struct vpul_subchain *other_sc;
	__u32 orig_seq_id;
	__u32 i, j;
	__s32 stat = VPU_STATUS_SUCCESS;

	for (i = 0; i < vtx->num_of_subchains; i++, sc++) {
		if (sc->stype != VPUL_SUB_CH_CPU_OP) {
			/* data in mailbox for all subchains sharing same seq_id must be
			 * written in sequence
			 */
			orig_seq_id = sc->sl_group_type_ident.seq_id;
			other_sc = first_sc;
			for (j = 0; j <= i; j++, other_sc++) {
				/* check if same seq_id already encountered */
				if (other_sc->sl_group_type_ident.seq_id == orig_seq_id)
					break;
			}
			/* if j < i : this seq-id has already be processed, do nothing */
			if (j == i) {
				/* 1st time encountering this seq_id : handle this subchain
				 * and all other subchains with same seq_id
				 */
				for (j = i; j < vtx->num_of_subchains; j++, other_sc++) {
					if (other_sc->sl_group_type_ident.seq_id == orig_seq_id) {
						stat = ta_check_update_subchains_pus(
								internal_context, proc_idx, j,
								task, other_sc, palloc);
						if (VPU_STATUS_IS_FAILURE(stat))
							return stat;
					}
				}
			}
		}
	}
	return stat;
}

static void get_mprb_info_from_port_bit_map(
	__u32 port_bit_map,
	struct mprb_entry * mprb_entry,
	struct VPUI_MprbGr ** ram_mprbs)
{
	__u32 i		= 0;
	__u32 bit_mask	= 1;
	__u32 current_mprb	= 0;
	__u32 last_mprb		= 0;
	/*init value*/
	(*ram_mprbs)->FstMprbId=0;
	(*ram_mprbs)->MprbsNum =0;
	for(;i < 8;i++){
		if((port_bit_map&bit_mask) != 0){
			/* 1 is added to the MPRB, since on MB we start to count from 1,
					not from 0 */
			current_mprb =  mprb_entry->preloaded_mprb[i] + 1;

			/* First MPRB found is added to first group */
			if((*ram_mprbs)->FstMprbId==0){

				(*ram_mprbs)->FstMprbId = current_mprb;
			}
			/* If not in sequence to last MPRB open new group */
			else if(last_mprb!= current_mprb-1 ){
				(*ram_mprbs)++;
				(*ram_mprbs)->FstMprbId = current_mprb;
			}
			(*ram_mprbs)->MprbsNum++;
			last_mprb = current_mprb;
		}
		bit_mask*=2;
	}
	(*ram_mprbs)++;
}



static __s32 ta_set_command_parameters(
	const struct vpul_task *task,
	struct VPUI_TaskAlloc *mb_ta,
	const __u8 *mb_cp
	)
{
	struct ptr_allocate_str palloc;
	const struct vpul_vertex *vtx = fst_vtx_ptr(task);
	const struct vpul_3dnn_process_base *proc_3dnn_base = fst_3dnn_process_base_ptr(task);
	__u32 i;
	__s32 stat = VPU_STATUS_SUCCESS;
	__u8 next_free_internal_unit = 0;

	struct internal_context internal_context;
	memset(&internal_context,0,sizeof(struct internal_context));

	/* set pointers */
	palloc.mb_ext_mem_addr = (struct VPUI_32bMem *)
		(mb_cp + mb_ta->DataOfs.ExternalMemAdrOfs);
	palloc.params = (__u16 *)(mb_cp + mb_ta->DataOfs.ParamsValuesOfs);
	palloc.bl_inst =
		(__u8 *)(mb_cp + mb_ta->DataOfs.BlInstances.DefaultOfs);
	palloc.hw_mprbs = (struct VPUI_MprbGr *)
		(mb_cp + mb_ta->DataOfs.HwMprbsGr.DefaultOfs);
	palloc.ram_mprbs = (struct VPUI_MprbGr *)
		(mb_cp + mb_ta->DataOfs.RamsMprbsGr.DefaultOfs);
	palloc.vir_unit =
		(__u8 *)(mb_cp + mb_ta->DataOfs.IntRamFstVirtUnit.DefaultOfs);

	for (i = 0; i < task->t_num_of_vertices; i++, vtx++) {
		if (vtx->vtype == VPUL_VERTEXT_PROC) {
			stat = ta_set_proc_pus(&internal_context, i, task, vtx, &palloc);
			if (VPU_STATUS_IS_FAILURE(stat))
				return stat;
		}
	}

	for (i = 0; i < task->t_num_of_3dnn_process_bases; i++, proc_3dnn_base++) {
		vtx = vertex_referencing_this_3dnn_proc_base(task, i);
		if (vtx) {
			stat = ta_set_3dnn_proc_pus(&internal_context, i, task, vtx, &palloc);
			if (VPU_STATUS_IS_FAILURE(stat))
				return stat;
		}
	}

	for(i = 0; i < task->n_external_mem_addresses;i++){
		ta_set_ex_mem(
			task->external_mem_addr[i],
			&palloc);
	}

	for (i = 0; i < task->n_memmap_desc;i++){
		const struct vpul_memory_map_desc * memd = &task->memmap_desc[i];
		switch( memd->mtype){
		case VPUL_MEM_EXTERNAL:
		{
			/*Nothing to do, external memory vector already set*/
			break;
		}
		case VPUL_MEM_PRELOAD_PU:
		{
			struct mprb_entry * mprb_entry = NULL;
			const struct vpul_image_size_desc * image_sizes
				= &( memd->image_sizes);
			__u8 m_vir_units = ((image_sizes->width*image_sizes->height
				*image_sizes->pixel_bytes)+VPUI_IN_RAM_VIRTUAL_UNIT_SIZE-1)
				/VPUI_IN_RAM_VIRTUAL_UNIT_SIZE;;
			ta_set_internal_mem(
				&palloc,
				&next_free_internal_unit,
				m_vir_units);
			mprb_entry = get_next_mprb_match(&internal_context,
				memd->pu_index.proc,
				memd->pu_index.sc,
				memd->pu_index.pu );
			if(mprb_entry==NULL){
				return VPU_STATUS_PRELOAD_MISMATCH;
			}
			do{

				get_mprb_info_from_port_bit_map(
					memd->pu_index.ports_bit_map,
					mprb_entry,
					&(palloc.ram_mprbs));


				mprb_entry = get_next_mprb_match(&internal_context,
					memd->pu_index.proc,
					memd->pu_index.sc,
					memd->pu_index.pu );

			}while(mprb_entry!=NULL);
			break;
		}
		case VPUL_MEM_INTERNAL:
		{
			__u32 mprb_group_index = task->internal_rams[memd->index].first_mprb_group_idx;
			__u32 counter = 0;
			const struct vpul_image_size_desc * image_sizes
				= &(task->memmap_desc[i].image_sizes);
			__u8 m_vir_units = ((image_sizes->width*image_sizes->height
				*image_sizes->pixel_bytes)+VPUI_IN_RAM_VIRTUAL_UNIT_SIZE-1)
				/VPUI_IN_RAM_VIRTUAL_UNIT_SIZE;;
			ta_set_internal_mem(
				&palloc,
				&next_free_internal_unit,
				m_vir_units);
			for(;counter< task->internal_rams[memd->index].n_mprb_groups;
				counter++,mprb_group_index++){
				palloc.ram_mprbs->FstMprbId =
					task->mprbs_groups[mprb_group_index].first_mprb_idx;
				palloc.ram_mprbs->MprbsNum =
					task->mprbs_groups[mprb_group_index].group_n_mprbs;
				palloc.ram_mprbs++;

			}
			break;
		}
		default:
		{
			stat = VPU_STATUS_FAILURE;
		}
		}
	}

	/* vir_unit parameters are u8 so we need to make sure data write is
	aligned for validation purpose */
	PAD_U8_PTR_TO_ALIGN(palloc.vir_unit);



	return stat;
}

static __u32 calc_mb_task_invoke_msg_size(const struct vpul_task *task)
{
	__u32 psize = 0;

	psize += m2size(calc_mb_create_invoke_param_size_from_task(task) * sizeof(__u16));
	psize += m2size(task->task_ro.n_invoke_extmem_addr *
		sizeof(struct VPUI_32bMem));

	return  sizeof(struct VPUM_HostCmndHdr) +
		sizeof(struct VPUI_CmndHdr) +
		sizeof(struct VPUI_TaskInvoke) +
		psize;
}

static __s32 ti_set_command(
	const struct vpul_task *task,
	__u32 invoaction_id,
	struct VPUI_TaskInvoke *mb_ti
	)
{
	__u32 cp_size;
	__u32 invoke_params_m2size=
		m2size(calc_mb_create_invoke_param_size_from_task(task)*sizeof(__u16));
	__u32 invoke_extmem_addr_m2size =
		m2size(task->task_ro.n_invoke_extmem_addr *sizeof(struct VPUI_32bMem));

	mb_ti->InvokeIdLsb = invoaction_id & 0xFFFF;
	mb_ti->InvokeIdMsb = (invoaction_id >> 16) & 0xFFFF;
	mb_ti->DataOfs.ParamsValues = 0; /* param values are located imm after struct end,ofs=0 */
	mb_ti->DataOfs.MemAddresses = invoke_params_m2size;

	cp_size  =
		invoke_params_m2size +
		invoke_extmem_addr_m2size;

	mb_ti->EntireDateSize =
		sizeof(struct VPUI_CmndHdr   ) +
		sizeof(struct VPUI_TaskInvoke) +
		cp_size;

	return VPU_STATUS_SUCCESS;

}

static __s32 ti_set_command_parameters(
	const struct vpul_task *task,
	const union vpul_pu_parameters *invoke_pu_params_vector,
	struct VPUI_TaskInvoke *mb_ti,
	__u8 *mb_cp
	)
{
	return generate_and_wr_invoke_parms_vector(task, invoke_pu_params_vector, mb_cp);
}

static __s32 ti_set_command_mem_addr(
	const struct vpul_task *task,
	const __u32 mem_addr_vec[],
	struct VPUI_TaskInvoke *mb_ti,
	__u8 *mb_cp
	)
{
	memcpy(mb_cp+mb_ti->DataOfs.MemAddresses, mem_addr_vec,
		sizeof(__u32)*task->task_ro.n_invoke_extmem_addr);
	return VPU_STATUS_SUCCESS;
}

void calc_mbox_messages_sizes(
	struct internal_context *internal_context,
	struct vpul_task *task,
	const struct VPUI_TaskCreate *mb_tc
	)
{
	task->task_ro.mbox_msg_sizes.allocate = calc_mb_task_allocate_msg_size(
		internal_context,
		task);
	task->task_ro.mbox_msg_sizes.invoke = calc_mb_task_invoke_msg_size(task);
	task->task_ro.mbox_msg_sizes.short_msg = sizeof(struct VPUM_HostCmndHdr) +
		sizeof(struct VPUI_CmndHdr);
}

/************************* VPU Library API functions *************************/


__s32 vpu_translator_init_core(
	const struct vpul_core_init_command_params *inf,
	size_t *mailbox_command_size,
	void *mailbox_command
	)
{
	__s32 stat;
	struct VPUI_Cmnd *mb_cmd;
	struct VPUI_InitCmnd *mb_init;
	__u32 mb_msg_size = sizeof(struct VPUM_HostCmndHdr) +
		sizeof(struct VPUI_CmndHdr) + sizeof(struct VPUI_InitCmnd);

	/* calculate message size */
	stat = check_size(mb_msg_size, mailbox_command_size, mailbox_command);
	if (stat == VPU_STATUS_NOP)
		stat = VPU_STATUS_SUCCESS;
	else
		return stat;

	/* check arguments */
	if (inf == NULL) {
		VPU_DEBG("inf is NULL.\n");
		return VPU_STATUS_BAD_PARAMS;
	}

	/* command header */
	mb_cmd = cmd_header(mailbox_command, VPUI_CMND_INIT, 0);

	/* message data information */
	mb_init = &mb_cmd->Extra.Init;
	set_mem_info(&inf->mem, &mb_init->Mem);
	set_time_info(&inf->time, &mb_init->Time);
	set_mbox_info(&inf->mbox, &mb_init->Mbox);

	return VPU_STATUS_SUCCESS;
}

__s32 vpu_translator_reallocate_mbox(
struct vpul_mbox_reallocate *realloc_info,
	size_t *mailbox_command_size,
	void *mailbox_command
	)
{
	__s32 stat;
	struct VPUI_Cmnd *mb_cmd;
	struct VPUI_ReAllocMbox *mb_alloc;
	__u32 mb_msg_size = sizeof(struct VPUM_HostCmndHdr) +
		sizeof(struct VPUI_CmndHdr) +
		sizeof(struct VPUI_ReAllocMbox);

	/* calculate message size */
	stat = check_size(mb_msg_size, mailbox_command_size, mailbox_command);
	if (stat == VPU_STATUS_NOP)
		stat = VPU_STATUS_SUCCESS;
	else
		return stat;

	/* check arguments */
	if (realloc_info == NULL) {
		VPU_DEBG("realloc_info is NULL.\n");
		return VPU_STATUS_BAD_PARAMS;
	}

	/* command header */
	mb_cmd = cmd_header(mailbox_command,
		VPUI_CMND_REALLOC_LOWPR_CMD_MBX, 0);

	/* message data information */
	mb_alloc = &mb_cmd->Extra.AllocMbox;
	mb_alloc->NewSize16b = realloc_info->new_host_normal_size_in_u16;

	return VPU_STATUS_SUCCESS;
}

__s32 vpu_translator_task_create(
struct vpul_task *task,
	size_t *mailbox_command_size,
	void *mailbox_command
	)
{
	return vpu_translator_task_create_w_mem_mark(
		task, NULL, mailbox_command_size, mailbox_command);
}
__s32 vpu_translator_task_create_w_mem_mark(
	struct vpul_task *task,
	const struct vpu_mark_for_invoke_ext_mem_vec_ds *invoke_mem_mark,
	size_t *mailbox_command_size,
	void *mailbox_command
	)
{
	__s32 stat;
	struct VPUI_Cmnd *mb_cmd;
	struct VPUI_TaskCreate *mb_tc;
	__u8 *mb_cp;
	__u32 mb_cp_size;
	__u32 mb_msg_size;
	struct internal_context internal_context;
	memset(&internal_context, 0, sizeof(struct internal_context));

	if (task == NULL)
		return VPU_STATUS_BAD_PARAMS;

	/* calculate Mbox message size */
	mb_msg_size = calc_mb_create_task_msg_size(task);
	stat = check_size(mb_msg_size, mailbox_command_size, mailbox_command);
	if (stat == VPU_STATUS_NOP)
		stat = VPU_STATUS_SUCCESS;
	else
		return stat;

	/* check for task validity */
	stat = check_task_ds(task);
	if (VPU_STATUS_IS_FAILURE(stat))
		return stat;

	/* command header */
	mb_cmd = cmd_header(mailbox_command, VPUI_CMND_CREATE_TASK, task->id);
	mb_tc = &mb_cmd->Extra.Create;
	mb_cp = ((__u8 *)(mb_tc)) + sizeof(struct VPUI_TaskCreate);

	/* set task header, command part (calculate total number of elements in
	 * MBox descriptor vectors) and processes descriptor.
	 */
	stat = set_task_create_command(
			&internal_context,
			task,
			invoke_mem_mark,
			mb_tc,
			mb_cp,
			&mb_cp_size);
	BUG_ON(VPU_STATUS_IS_FAILURE(stat));

	*mailbox_command_size =
		sizeof(struct VPUM_HostCmndHdr) + mb_tc->EntireDateSize;

	/* command parameters: from here code assumes that command section and
	 * processes descriptors on "command parameters" section are set
	 */

	/* task level descriptors: vertices, edges, edges groups and loops */
	stat = set_task_level_descriptors(&internal_context,task, mb_tc, mb_cp);
	BUG_ON(VPU_STATUS_IS_FAILURE(stat));

	if (invoke_mem_mark) {
		stat = set_task_invoke_mem_indc(task,
			invoke_mem_mark, mb_tc, mb_cp);
		BUG_ON(VPU_STATUS_IS_FAILURE(stat));
	}

	/* set all descriptors of all processes */
	if (VPU_STATUS_IS_SUCCESSFUL(stat)){
		stat = set_process_level_descriptors(&internal_context, task, mb_tc, mb_cp);
	}

	/* mem descriptor needs to be set after PU so we can connect to PU MPRBS
	   if needed */
	if (VPU_STATUS_IS_SUCCESSFUL(stat)){
		stat= set_task_mem_desc(&internal_context, task,  mb_tc, mb_cp);
	}
	if (VPU_STATUS_IS_SUCCESSFUL(stat)){
		/* save internal information on task data structure */
		save_internal_info(task, mb_tc, mb_cp);

		/* calculate mbox messages sizes */
		calc_mbox_messages_sizes(
		&internal_context,
		task, mb_tc);
	}
	return stat;
}

__s32 vpu_translator_task_allocate(
	const struct vpul_task *task,
	__u32 number_of_slots,
	size_t *mailbox_command_size,
	void *mailbox_command
	)
{
	__s32 stat;
	struct VPUI_Cmnd *mb_cmd;
	struct VPUI_TaskAlloc *mb_ta;
	__u8 *mb_cp;
	__u32 mb_msg_size;

	if (task == NULL)
		return VPU_STATUS_BAD_PARAMS;

	mb_msg_size = task->task_ro.mbox_msg_sizes.allocate;
	stat = check_size(mb_msg_size, mailbox_command_size, mailbox_command);
	if (stat == VPU_STATUS_NOP)
		stat = VPU_STATUS_SUCCESS;
	else
		return stat;

	/* command header */
	mb_cmd = cmd_header(mailbox_command, VPUI_CMND_ALLOCATE_TASK, task->id);

	mb_ta = &mb_cmd->Extra.Allocate;
	mb_cp = (__u8 *)(mb_ta) + sizeof(struct VPUI_TaskAlloc);

	stat = ta_set_command(task, number_of_slots, mb_ta);
	if (stat != VPU_STATUS_SUCCESS)
		return stat;

	stat = ta_set_command_parameters(task, mb_ta, mb_cp);

	return stat;
}

__s32 vpu_translator_task_invoke(
	const struct vpul_task *task,
	__u32 invoaction_id,
	const union vpul_pu_parameters *invoke_pu_params_vector,
	const struct vpu_invoke_external_mem_vector_ds *invoke_memories_vector,
	size_t *mailbox_command_size,
	void *mailbox_command
	)
{
	__s32 stat;
	struct VPUI_Cmnd *mb_cmd;
	struct VPUI_TaskInvoke *mb_ti;
	__u8 *mb_cp;
	__u32 mb_msg_size;

	if (task == NULL)
		return VPU_STATUS_BAD_PARAMS;


	/* calculate Mbox message size */
	mb_msg_size = task->task_ro.mbox_msg_sizes.invoke;
	stat = check_size(mb_msg_size, mailbox_command_size, mailbox_command);
	if (stat == VPU_STATUS_NOP)
		stat = VPU_STATUS_SUCCESS;
	else
		return stat;

	/* check command arguments */
	if (invoaction_id == 0)	{
		VPU_DEBG("invocation ID must be != 0");
		return VPU_STATUS_BAD_PARAMS;
	}

	/* command header */
	mb_cmd = cmd_header(mailbox_command, VPUI_CMND_INVOKE_TASK, task->id);

	/* general information */
	mb_ti = &mb_cmd->Extra.Invoke;
	mb_cp = (__u8 *)(mb_ti) + sizeof(struct VPUI_TaskInvoke);

	stat = ti_set_command(task, invoaction_id, mb_ti);
	BUG_ON(VPU_STATUS_IS_FAILURE(stat));

	if (VPU_STATUS_IS_SUCCESSFUL(stat)) {
		stat = ti_set_command_parameters(
					task,
					invoke_pu_params_vector,
					mb_ti,
					mb_cp);
		BUG_ON(VPU_STATUS_IS_FAILURE(stat));
	}
	if (VPU_STATUS_IS_SUCCESSFUL(stat) && invoke_memories_vector != NULL) {
		if (invoke_memories_vector->num_of_buffers !=
			task->task_ro.n_invoke_extmem_addr ) {
			stat = VPU_STATUS_BAD_PARAMS;
		} else {
			stat = ti_set_command_mem_addr(
				task,
				invoke_memories_vector->addresses_vector,
				mb_ti,
				mb_cp);
		}
	}

	return stat;
}

__s32 vpu_translator_task_update_parameters(
	const struct vpul_task *task,
	__u32 number_of_parameters_to_update,
	struct vpu_update_parameters_vector_ds *update_parameters_vector,
	size_t *mailbox_command_size,
	void *mailbox_command
	)
{
	__s32 stat, mb_msg_size;
	struct VPUI_Cmnd *mb_cmd;
	struct VPUI_TaskUpdateParams *mb_up;

	if (task == NULL)
		return VPU_STATUS_BAD_PARAMS;

	if ((update_parameters_vector == NULL) &&
		(number_of_parameters_to_update != 0))
		return VPU_STATUS_BAD_PARAMS;

	/* calculate Mbox message size */
	mb_msg_size = VPU_MAX_MB_CREATE_TASK_PARAM_SIZE; /* TODO: calc size */
	stat = check_size(mb_msg_size, mailbox_command_size, mailbox_command);
	if (stat == VPU_STATUS_NOP)
		stat = VPU_STATUS_SUCCESS;
	else
		return stat;

	/* command header */
	mb_cmd = cmd_header(mailbox_command, VPUI_CMND_UPDATE_TASK_PARAMS,
		task->id);

	mb_up = &mb_cmd->Extra.UpdateParams;
	memset(mb_up, 0, sizeof(struct VPUI_TaskUpdateParams)); /* TODO:?*/

	return stat;
}


__s32 vpu_translator_task_destroy(
	const struct vpul_task *task,
	size_t *mailbox_command_size,
	void *mailbox_command
	)
{
	return prepare_short_mb_command(task, mailbox_command_size,
		mailbox_command, VPUI_CMND_DESTROY_TASK);
}

__s32 vpu_translator_task_abort(
	const struct vpul_task *task,
	size_t *mailbox_command_size,
	void *mailbox_command
	)
{
	return prepare_short_mb_command(task, mailbox_command_size,
		mailbox_command, VPUI_CMND_ABORT_TASK);
}

__s32 vpu_translator_task_free(
	const struct vpul_task *task,
	size_t *mailbox_command_size,
	void *mailbox_command
	)
{
	return prepare_short_mb_command(task, mailbox_command_size,
		mailbox_command, VPUI_CMND_FREE_TASK);
}
