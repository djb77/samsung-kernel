/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _UH_INFORM_H
#define _UH_INFORM_H

/*RETURN VALUE*/
#define UH_INFORM_TRUE 0
#define UH_INFORM_FALSE 1

/*Feature ID*/
enum {
	/* Feature region */
	FEATURE_ROPP,
	FEATURE_JOPP,
	FEATURE_RKP,
	FEATURE_KDP,
	FEATURE_DMV,

	/* DMV region */
	DMV_MOUNTED_ODM,
	DMV_MOUNTED_SYSTEM,
	DMV_MOUNTED_VENDOR,
	UH_INFORM_SIZE_FEATURE
};

/*Feature STATE*/
enum {
	UH_INFORM_INACTIVE,
	UH_INFORM_ACTIVE,
	UH_INFORM_DMV_ALTA,
	UH_INFORM_DMV_LITE,
	UH_INFORM_DMV_FULL,
	DMV_MOUNTED,
	DMV_UNMOUNTED,
	UH_INFORM_INFORM_ERROR,
	UH_INFORM_SIZE_STATE
};

/*Feature TYPE*/
enum{
	UH_INFORM_NORMAL_FEATURE,
	UH_INFORM_DMV_FEATURE,
	UH_INFORM_PARTITION,
	UH_INFORM_MODE,
	UH_INFORM_SIZE_TYPE
};

/*MODE*/
enum {
	MODE_PRINT,
	MODE_WRITE
};


int feature_set_state(int feature_id, int feature_state);

static int feature_init(int feature_id, int feature_type, int feature_state, char* feature_id_str );
static int feature_get_config(void);
static int uh_inform_init(void);
static int find_mount_dir(int feature_id);
static char *get_feature_id_str(int feature_id);
static char *get_feature_state_str(int feature_id);
static int get_partition_info(void);
static void print_feature(struct seq_file *m);

#endif //_UH_INFORM_H

