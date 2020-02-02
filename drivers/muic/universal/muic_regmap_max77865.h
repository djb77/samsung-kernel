#ifndef _MUIC_REGMAP_MAX77865_
#define _MUIC_REGMAP_MAX77865_

#define MAX77865_REG_INT_MAIN       (0x01)
#define MAX77865_REG_INT_BC         (0x02)
#define MAX77865_REG_INT_FC         (0x03)
#define MAX77865_REG_INT_GP         (0x04)
#define INT_MAIN_CLEAR              (0x1A)
#define INT_BC_CLEAR                (0xFF)
#define INT_FC_CLEAR                (0xFC)
#define INT_GP_CLEAR                (0x1F)

#define MAX77865_VBUS_MASK          (0x1 << 7)
#define MAX77865_OVP_MASK           (0x1 << 6)
#define MAX77865_DCDTMR_MASK        (0x1 << 1)
#define MAX77865_RESET_MASK         (0x1 << 1)

#define MAX77865_REG_INTMASK_MAIN   (0x07)
#define MAX77865_REG_INTMASK_BC     (0x08)
#define MAX77865_REG_INTMASK_FC     (0x09)
#define MAX77865_REG_INTMASK_GP     (0x0A)
#define INTMASK_MAIN_RESTORE        (0x05)
#define INTMASK_BC_RESTORE          (0x00)
#define INTMASK_FC_RESTORE          (0x77)
#define INTMASK_GP_RESTORE          (0xE0)

#define MAX77865_REG_CCDET          (0x15)
#define CCDET_RESTORE               (0x00)

#define MAX77865_MUIC_REG_CONTROL2  (0x1A)
#define CONTROL2_CPEN_SHIFT         2
#define CONTROL2_CPEN_MASK          (0x1 << CONTROL2_CPEN_SHIFT)
#define CONTROL2_CPEN_ENABLE        (0x25)
#define CONTROL2_CPEN_DISABLE       (0x21)

#endif
