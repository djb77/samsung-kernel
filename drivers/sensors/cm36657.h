#ifndef __LINUX_cm36657_H

#define __cm36657_H__
#include <linux/types.h>

#ifdef __KERNEL__

/*cm36657*/
/*for ALS CONF command*/
#define CM36657_CS_START	(1 << 7)
#define CM36657_CS_IT_50MS	(0 << 2)
#define CM36657_CS_IT_100MS	(1 << 2)
#define CM36657_CS_IT_200MS	(2 << 2)
#define CM36657_CS_IT_400MS	(3 << 2)

#define CM36657_CS_SD		(1 << 0) /*enable/disable ALS func, 1:disable , 0: enable*/
#define CM36657_CS_SD_MASK	0xFFFE

/*for PS CONF1 command*/
#define CM36657_PS_START	(1 << 11)

#define CM36657_PS_INT_ENABLE	   (2 << 2) /*enable/disable Interrupt*/
#define CM36657_PS_INT_MASK        0xFFF3 /*enable/disable Interrupt*/

#define CM36657_PS_PERS_1	   (0 << 4)
#define CM36657_PS_PERS_2	   (1 << 4)
#define CM36657_PS_PERS_3	   (2 << 4)
#define CM36657_PS_PERS_4	   (3 << 4)
#define CM36657_PS_IT_1T	     (0 << 14)
#define CM36657_PS_IT_2T	     (1 << 14)
#define CM36657_PS_IT_4T	     (2 << 14)
#define CM36657_PS_IT_8T	     (3 << 14)
#define CM36657_PS_SMART_PERS  (1 << 1)
#define CM36657_PS_SD	         (1 << 0) /*enable/disable PS func, 1:disable , 0: enable*/
#define CM36657_PS_SD_MASK     0xFFFE

#define CM36657_PS_PERIOD_8_MASK    0xFF3F /* for prox cal */

/*for PS CONF3 command*/
#define CM36657_LED_I_70              (0 << 8)
#define CM36657_LED_I_95              (1 << 8)
#define CM36657_LED_I_110             (2 << 8)
#define CM36657_LED_I_130             (3 << 8)
#define CM36657_LED_I_170             (4 << 8)
#define CM36657_LED_I_200             (5 << 8)
#define CM36657_LED_I_220             (6 << 8)
#define CM36657_LED_I_240             (7 << 8)
#define CM36657_PS_ACTIVE_FORCE_MODE  (1 << 6)
#define CM36657_PS_ACTIVE_FORCE_TRIG  (1 << 5)
#define CM36657_PS_START2             (1 << 3)

extern struct class *sensors_class;
#endif
#endif
