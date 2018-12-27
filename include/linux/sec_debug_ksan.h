/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef SEC_DEBUG_KSAN_H
#define SEC_DEBUG_KSAN_H

enum {
	UBSAN_RTYPE_OVERFLOW =1, //handle_overflow
	UBSAN_RTYPE_NEGATE_OVERFLOW, //__ubsan_handle_negate_overflow
	UBSAN_RTYPE_DIVREM_OVERFLOW, //__ubsan_handle_divrem_overflow
	UBSAN_RTYPE_NULL_PTR_DEREF, //handle_null_ptr_deref
	UBSAN_RTYPE_MISALIGNED_ACCESS, //handle_missaligned_access
	UBSAN_RTYPE_OBJECT_SIZE_MISMATCH, //handle_object_size_mismatch
	UBSAN_RTYPE_NONNULL_RETURN, //__ubsan_handle_nonnull_return
	UBSAN_RTYPE_VLA_BOUND_NOT_POSITIVE, //__ubsan_handle_vla_bound_not_positive
	UBSAN_RTYPE_OUT_OF_BOUND, //__ubsan_handle_out_of_bounds
	UBSAN_RTYPE_SHIFT_OUT_OF_BOUND, //__ubsan_handle_shift_out_of_bounds
	UBSAN_RTYPE_LOAD_INVALID_VALUE =11, //__ubsan_handle_load_invalid_value
	UBSAN_RTYPE_CNT	
};

enum {
	KASAN_RTYPE_OUT_OF_BOUND = 1,
	KASAN_RTYPE_SLAB_OUT_OF_BOUND,
	KASAN_RTYPE_GLOBAL_OUT_OF_BOUND,
	KASAN_RTYPE_STACK_OUT_OF_BOUND,
	KASAN_RTYPE_USE_AFTER_FREE,
	KASAN_RTYPE_USE_AFTER_SCOPE = 6,
	KASAN_RTYPE_CNT
};


#define FILESIZE 100 
#define ALLTYPE  100


struct kasandebug {
	char ap[FILESIZE]; //access point
	int rType;
	struct list_head entry;
};

struct ubsandebug {
	char filename[FILESIZE];
	int rType;
	struct list_head entry;
};


extern void sec_debug_ubsan_handler(struct ubsandebug*);
extern void sec_debug_kasan_handler(struct kasandebug*);
#endif
