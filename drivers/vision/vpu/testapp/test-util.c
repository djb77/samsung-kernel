#include <stdio.h>
#include <stdarg.h>

#include "test-util.h"
#include <lib/vpul-errno.h>
#include <lib/vpul-ds.h>

int loglevel = 1;

int util_g_size_from(struct vs4l_format *format)
{
	unsigned int pixel_bytes;

	switch (format->format) {
	case VS4L_DF_IMAGE_U8:
		pixel_bytes = 1;
		break;
	case VS4L_DF_IMAGE_U16:
		pixel_bytes = 2;
		break;
	case VS4L_DF_IMAGE_RGB:
		pixel_bytes = 3;
		break;
	case VS4L_DF_IMAGE_U32:
		pixel_bytes = 4;
		break;
	default:
		TEST_LOG("invalid format(%d)\n", format->format);
		break;
	}

	return pixel_bytes * format->width * format->height;
}

int util_memdump16(unsigned short *start,
	unsigned short *end)
{
	int ret = 0;
	unsigned short *cur;
	unsigned short items, offset;
	char term[50], sentence[250];

	cur = start;
	items = 0;
	offset = 0;

	memset(sentence, 0, sizeof(sentence));
	snprintf(sentence, sizeof(sentence), "[V] Memory Dump(%p ~ %p)", start, end);

	while (cur < end) {
		if ((items % 16) == 0) {
			printf("%s\n", sentence);
			offset = 0;
			items = 0;
		}

		snprintf(term, sizeof(term), "0x%04X ", *cur);
		snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
		offset += strlen(term);
		cur++;
		items++;
	}

	if (items) {
		printf("%s\n", sentence);
	}

	ret = cur - end;

	return ret;
}

__u32 vpu_translator_get_task_ds_size(
	__u32 t_num_of_vertices,
	__u32 t_num_of_subchains,
	__u32 t_num_of_pus,
	__u32 t_num_of_updatable_pus
	)
{
	__u32 ret = 0;

	if ((t_num_of_vertices != 0) && (t_num_of_subchains != 0) &&
		(t_num_of_pus != 0)) {
		/* Calculation's is done top down .For each level we need
		 * to calculate the size of static structure times the number
		 * of instances
		 */
		ret += sizeof(struct vpul_task);
		ret += t_num_of_vertices * sizeof(struct vpul_vertex);
		ret += t_num_of_subchains * sizeof(struct vpul_subchain);
		ret += t_num_of_pus * sizeof(struct vpul_pu);
		ret += t_num_of_updatable_pus *
								sizeof(struct vpul_pu_location);

	}
	return ret;
}

__s32 vpu_translator_create_task_ds(
	struct vpul_task *task,
	__u32 size_allocated,
	__u32 t_num_of_vertices,
	__u32 t_num_of_subchains,
	__u32 t_num_of_pus,
	__u32 t_num_of_updatable_pus,
	...)
{
	__s32 status = VPU_STATUS_SUCCESS;
	__u32 nsize = vpu_translator_get_task_ds_size(t_num_of_vertices,
		t_num_of_subchains, t_num_of_pus, t_num_of_updatable_pus);
	__u32 i = 0;
	__u32 j = 0;
	va_list a_list;
	struct vpul_vertex *vtx;
	__u32 curr_sc_line_ofs;
	__u32 checkSumCount = 0;

	if ((task == NULL) ||
		(nsize == 0) ||
		(size_allocated < nsize)) {
		status = VPU_STATUS_BAD_PARAMS;
		return status;
	}

	memset(task, 0, nsize);
	task->total_size = nsize;

	/* initialize task structure */
	task->t_num_of_vertices = t_num_of_vertices;
	task->vertices_vec_ofs = sizeof(struct vpul_task);

	task->sc_vec_ofs = task->vertices_vec_ofs +
		(sizeof(struct vpul_vertex) * t_num_of_vertices);
	task->pus_vec_ofs = task->sc_vec_ofs
		+ (sizeof(struct vpul_subchain) * t_num_of_subchains);
	task->invoke_params_vec_ofs = task->pus_vec_ofs
		+ (sizeof(struct vpul_pu) * t_num_of_pus);

	task->t_num_of_subchains = t_num_of_subchains;
	task->t_num_of_pus = t_num_of_pus;
	task->t_num_of_pu_params_on_invoke = t_num_of_updatable_pus;
	va_start(a_list, t_num_of_updatable_pus) /* last arg on list */;

	/* initialize vertices */
	vtx = fst_vtx_ptr(task);
	curr_sc_line_ofs = task->sc_vec_ofs;

	for (i = 0; i < t_num_of_vertices; i++) {
		vtx->num_of_subchains = va_arg(a_list, __u32);
		checkSumCount += vtx->num_of_subchains;
		vtx->sc_ofs = curr_sc_line_ofs;
		curr_sc_line_ofs += sizeof(struct vpul_subchain) *
			vtx->num_of_subchains;
		vtx++;
	}

	if (checkSumCount != task->t_num_of_subchains)
		return VPU_STATUS_BAD_PARAMS;

	checkSumCount = 0;

	/* initialize sub-chains */
	vtx = fst_vtx_ptr(task);
	for (i = 0; i < t_num_of_vertices; i++, vtx++) {
		struct vpul_subchain *sc = fst_vtx_sc_ptr(task, vtx);
		__u32 pus_line_ofs = task->pus_vec_ofs;

		for (j = 0; j < vtx->num_of_subchains; j++, sc++) {
			sc->num_of_pus = va_arg(a_list, __u32);
			checkSumCount += sc->num_of_pus;
			sc->pus_ofs = pus_line_ofs;
			pus_line_ofs +=
				sizeof(struct vpul_pu) * sc->num_of_pus;
		}
	}
	va_end(a_list);

	if (checkSumCount != task->t_num_of_pus)
		return VPU_STATUS_BAD_PARAMS;

	return status;
}

struct vpul_pu * get_pu_by_index(
	const struct vpul_task * task,
	__u32 proc_idx,
	__u32 sc_idx,
	__u32 pu_idx)
{
	struct vpul_subchain * sc;
	struct vpul_pu * pu;
	struct vpul_vertex * vtx = (struct vpul_vertex *)((__u8*)task+(task)->vertices_vec_ofs);
	vtx += proc_idx;
	sc = (struct vpul_subchain *)((__u8*)task+(vtx)->sc_ofs);
	sc += sc_idx;
	pu = (struct vpul_pu *)((__u8*)task+(sc)->pus_ofs);
	pu += pu_idx;
	return pu;
}

__s32 vpu_translator_create_task_ds_from_array(
	struct vpul_task *task,
	__u32 size_allocated,
	__u32 t_num_of_vertices,
	__u32 t_num_of_subchains,
	__u32 t_num_of_pus,
	__u32 t_num_of_updatable_pus,
	const __u32 *subchains_for_vertices,
	const __u32 *pus_for_subchains)
{
	__s32 status = VPU_STATUS_SUCCESS;
	__u32 nsize = vpu_translator_get_task_ds_size(t_num_of_vertices,
		t_num_of_subchains, t_num_of_pus, t_num_of_updatable_pus);
	__u32 i = 0;
	__u32 j = 0;

	struct vpul_vertex *vtx;
	__u32 curr_sc_line_ofs;
	__u32 checkSumCount = 0;
	__u32 task_sc_idx = 0;
	__u32 pus_line_ofs;

	if ((task == NULL) || (subchains_for_vertices == NULL) ||
		(pus_for_subchains == NULL) || (nsize == 0) ||
		(size_allocated < nsize)) {
		status = VPU_STATUS_BAD_PARAMS;
		return status;
	}

	memset(task, 0, nsize);
	task->total_size = nsize;

	/* initialize task structure */
	task->t_num_of_vertices = t_num_of_vertices;
	task->vertices_vec_ofs = sizeof(struct vpul_task);
	task->sc_vec_ofs = task->vertices_vec_ofs +
		(sizeof(struct vpul_vertex) * t_num_of_vertices);
	task->pus_vec_ofs = task->sc_vec_ofs
		+ (sizeof(struct vpul_subchain) * t_num_of_subchains);

	task->t_num_of_subchains = t_num_of_subchains;
	task->t_num_of_pus = t_num_of_pus;

	/* initialize vertices */
	vtx = fst_vtx_ptr(task);
	curr_sc_line_ofs = task->sc_vec_ofs;

	for (i = 0; i < t_num_of_vertices; i++) {
		vtx->num_of_subchains = subchains_for_vertices[i];
		checkSumCount += vtx->num_of_subchains;
		vtx->sc_ofs = curr_sc_line_ofs;
		curr_sc_line_ofs += sizeof(struct vpul_subchain) *
			vtx->num_of_subchains;
		vtx++;
	}

	if (checkSumCount != task->t_num_of_subchains)
		return VPU_STATUS_BAD_PARAMS;

	checkSumCount = 0;
	pus_line_ofs = task->pus_vec_ofs;

	/* initialize sub-chains */
	vtx = fst_vtx_ptr(task);
	for (i = 0; i < t_num_of_vertices; i++, vtx++) {
		struct vpul_subchain *sc = fst_vtx_sc_ptr(task, vtx);

		for (j = 0; j < vtx->num_of_subchains; j++, sc++) {
			sc->num_of_pus = pus_for_subchains[task_sc_idx];
			task_sc_idx++;
			checkSumCount += sc->num_of_pus;
			sc->pus_ofs = pus_line_ofs;
			pus_line_ofs +=
				sizeof(struct vpul_pu) * sc->num_of_pus;
		}
	}

	if (checkSumCount != task->t_num_of_pus)
		return VPU_STATUS_BAD_PARAMS;

	return status;
}
