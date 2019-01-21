/*
 * =====================================================================================
 *
 *       Filename:  Iccc_Interface.h
 *
 *    Description:  This header file must be in sync with other platform, tz related files
 *
 *        Version:  1.0
 *        Created:  08/20/2015 12:13:42 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Areef Basha (), areef.basha@samsung.com
 *        Company:  Samsung Electronics
 *
 *        Copyright (c) 2015 by Samsung Electronics, All rights reserved.
 *
 * =====================================================================================
 */
#ifndef Lksecapp_Interface_H_
#define Lksecapp_Interface_H_

#if defined(CONFIG_TZ_ICCC)
#define	MAX_IMAGES      6
#define RESERVED_BYTES	96
#endif

#define SEC_BOOT_CMD100_SET_DMV_DATA 100
#define SEC_BOOT_CMD101_GET_DMV_DATA 101
#define SHA256_SIZE 32

typedef struct {
    uint16_t magic_str;
    uint16_t used_size;
} secure_param_header_t;

#if defined(CONFIG_TZ_ICCC)
typedef struct {
    secure_param_header_t header;
    uint32_t rp_ver;                    // possible values : 0,1,2..
    uint32_t kernel_rp;                 // possible values : 0,1,2..0xFFFFFFFF (0,1,2 ... : kernel RP version , 0XFFFFFFFF :feature not present)
    uint32_t system_rp;                 // possible values : 0,1,2..0xFFFFFFFF (0,1,2 ... : system RP version , 0XFFFFFFFF :feature not present)
    uint32_t test_bit;                  // possible values : 0,1 (0: not blown , 1:blown)
    uint32_t sec_boot;                  // possible values : 0,1 (0:disable , 1:enable)
    uint32_t react_lock;                // possible values : 0,1,0xFFFFFFFF(0:off , 1:on , 0xFFFFFFFF:feature not present)
    uint32_t kiwi_lock;                 // possible values : 0,1,0xFFFFFFFF(0:off , 1:on , 0xFFFFFFFF:feature not present)
    uint32_t frp_lock;                  // possible values : 0,1,0xFFFFFFFF(0:off , 1:on , 0xFFFFFFFF:feature not present)
    uint32_t cc_mode;                   // possible values : 0,1,0xFFFFFFFF(0:off , 1:on , 0xFFFFFFFF:feature not present)
    uint32_t mdm_mode;                  // possible values : 0,1,0xFFFFFFFF(0:off , 1:on , 0xFFFFFFFF:feature not present)
    uint32_t curr_bin_status;           // possible values : 0,1 (0:official , 1:custom)
    uint32_t afw_value;                 // possible values : 0,1,0xFFFFFFFF (0:off , 1:on, 0xFFFFFFFF: Unknown/Uninitialized)
    uint32_t warranty_bit;              // possible values : 0,1 (0: not blown , 1:blown)
    uint32_t kap_status;                // possible values : 0,1,3,0xFFFFFFFF(0:KAP OFF , 1:KAP ON - No tamper , 3:Violation(RKP and/or DMVerity says device is tampered) , 0xFFFFFFFF:feature not present)
    uint32_t image_status[MAX_IMAGES];  // possible values : 0,1,0xFF (0:official , 1:custom , 0xFFFFFFFF : FRP not present so cannot get image status)
    uint8_t reserved[RESERVED_BYTES];
} __attribute__ ((packed)) iccc_bl_status_t;
#endif

typedef struct trusted_boot_bl_status_s
{
    unsigned char hash[SHA256_SIZE];
} __attribute__ ((packed)) trusted_boot_bl_status_t;

typedef struct dmv_bl_status_s
{
    secure_param_header_t header;
    uint32_t odin_flag;
    uint32_t boot_mode;
    uint32_t security_mode;
	uint32_t mode_num;
} __attribute__ ((packed)) dmv_bl_status_t;

typedef struct dmv_req_s
{
    uint32_t cmd_id;
    union {
#if defined(CONFIG_TZ_ICCC)
		iccc_bl_status_t iccc_bl_status;
#endif
		dmv_bl_status_t dmv_bl_status ;
		trusted_boot_bl_status_t bl_secure_info3;
    } data;
} dmv_req_t;

typedef struct dmv_rsp_s
{
  /** First 4 bytes should always be command id */
  uint32_t data;
  uint32_t status;
}dmv_rsp_t;

#endif /* Lksecapp_Interface_H_ */
