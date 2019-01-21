#ifndef __APP_DMVERITY_H
#define __APP_DMVERITY_H

#define SEC_BOOT_CMD100_SET_DMV_DATA 100
#define SEC_BOOT_CMD101_GET_DMV_DATA 101
#define QSEECOM_SBUFF_SIZE 0x1000

#ifndef _TZ_ICCC_COMDEF_H_
#define MAX_IMAGES  4
#define RESERVED_BYTES  96

typedef struct {
    uint16_t magic_str;
    uint16_t used_size;
} secure_param_header_t;

typedef struct
{
    secure_param_header_t header;
    uint32_t rp_ver;
    uint32_t kernel_rp;
    uint32_t system_rp;
    uint32_t test_bit;
    uint32_t sec_boot;
    uint32_t react_lock;
    uint32_t kiwi_lock;
    uint32_t cc_mode;
    uint32_t mdm_mode;
    uint32_t sysscope_flag;
    uint32_t trustboot_flag;
    uint32_t afw_value;
    uint8_t image_status[MAX_IMAGES];
    uint8_t reserved[RESERVED_BYTES];
} __attribute__ ((packed)) iccc_bl_status_t;
#endif

typedef struct dmv_bl_status_s
{
    secure_param_header_t header;
    uint32_t odin_flag;
    uint32_t boot_mode;
    uint32_t security_mode;
} __attribute__ ((packed)) dmv_bl_status_t;

typedef struct dmv_req_s
{
    uint32_t cmd_id;
    union {
		iccc_bl_status_t iccc_bl_status;
		dmv_bl_status_t dmv_bl_status ;
    } data;
} dmv_req_t;

typedef struct dmv_rsp_s
{
  /** First 4 bytes should always be command id */
  uint32_t data;
  uint32_t status;
}dmv_rsp_t;
#endif
