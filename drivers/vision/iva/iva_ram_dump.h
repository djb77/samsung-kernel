#ifndef _IVA_RAMDUMP_H_
#define _IVA_RAMDUMP_H_

#include "iva_ctrl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int32_t iva_ram_dump_init(struct iva_dev_data *iva);
extern int32_t iva_ram_dump_deinit(struct iva_dev_data *iva);

#ifdef __cplusplus
}
#endif
#endif /* _IVA_RAMDUMP_H_ */
