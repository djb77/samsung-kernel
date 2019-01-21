/*
 * SELF DISLAY interface
 *
 * Author: samsung display driver team
 * Company:  Samsung Electronics
 */

 #ifndef _UAPI_LINUX_SELF_DISPLAY_H_
 #define _UAPI_LINUX_SELF_DISPLAY_H_

 /*
  * ioctl
  */
#define SELF_DISPLAY_IOCTL_MAGIC		'S'
#define IOCTL_SELF_MOVE					_IOW(SELF_DISPLAY_IOCTL_MAGIC, 1, struct self_move_info)
#define IOCTL_SELF_MASK					_IOW(SELF_DISPLAY_IOCTL_MAGIC, 2, struct self_mask_info)
#define IOCTL_SELF_ICON_GRID			_IOW(SELF_DISPLAY_IOCTL_MAGIC, 3, struct self_icon_grid_info)
//#define IOCTL_SELF_GRID					_IOW(SELF_DISPLAY_IOCTL_MAGIC, 4, struct self_grid_info)
#define IOCTL_SELF_ANALOG_CLOCK			_IOW(SELF_DISPLAY_IOCTL_MAGIC, 4, struct self_analog_clock_info)
#define IOCTL_SELF_DIGITAL_CLOCK		_IOW(SELF_DISPLAY_IOCTL_MAGIC, 5, struct self_digital_clock_info)
#define IOCTL_SELF_BLINGKING			_IOW(SELF_DISPLAY_IOCTL_MAGIC, 6, struct self_blinking_info)
#define IOCTL_SELF_SYNC_OFF				_IOW(SELF_DISPLAY_IOCTL_MAGIC, 7, enum self_display_sync_off)
#define IOCTL_SELF_MAX					8

/*
 * data flag
 * This flag is uesd to distinguish what data will be passed and added at first 1byte of data.
 * ex ) ioctl write data : flag (1byte) + data (bytes)
 */
enum self_display_data_flag {
	FLAG_SELF_MOVE = 1,
	FLAG_SELF_MASK = 2,
	FLAG_SELF_ICON = 3,
	FLAG_SELF_GRID = 4,
	FLAG_SELF_ACLK = 5,
	FLAG_SELF_DCLK = 6,
	FLAG_SELF_DISP_MAX,
};

/*
 * Sync off
 */
enum self_display_sync_off {
	SELF_MOVE_SYNC_OFF = 0,
	SELF_ICON_GRID_SYNC_OFF,
	SELF_CLOCK_SYNC_OFF,
};

/*
 * self move struct
 */
struct self_move_info {
	u8 MV_OFF_SEL;
	u32 MOVE_DISP_X;
	u32 MOVE_DISP_Y;
	u8 SELF_MV_X[59];
	u8 SELF_MV_Y[59];
	u8 SI_MOVE_X[59];
	u8 SI_MOVE_Y[59];
};

/*
 * self icon struct
 */
struct self_icon_grid_info {
	u8 SI_OFF_SEL;
	u8 SI_ICON_EN;
	u32 SI_ST_X;
	u32 SI_ST_Y;
	u32 SI_IMG_WIGTH;
	u32 SI_IMG_HEIGHT;
	u8 SG_OFF_SEL;
	u8 SG_GRID_EN;
	u8 SG_BLEND_STEP;
	u32 SG_WIN_STP_X;
	u32 SG_WIN_STP_Y;
	u32 SG_WIN_END_X;
	u32 SG_WIN_END_Y;
};

/*
 * self grid struct
 */
 /*
struct self_grid_info {
	u8 SG_OFF_SEL;
	u8 SG_BLEND_STEP;
	u32 SG_WIN_STP_X;
	u32 SG_WIN_STP_Y;
	u32 SG_WIN_END_X;
	u32 SG_WIN_END_Y;
};
*/

/*
 * self mask struct
 */
struct self_mask_info {
	char SM_MASK_EN;
};

/*
 * self analog clock struct
 */
struct self_analog_clock_info {
	u8 SC_OFF_SEL;
	u8 SC_ROTATE;
	u8 SC_SET_HH;
	u8 SC_SET_MM;
	u8 SC_SET_SS;
	u8 SC_SET_MSS;
	u32 SC_CENTER_X;
	u32 SC_CENTER_Y;
	u8 SC_HMS_MASK;
	u8 SC_HMS_LAYER;
};

/*
* self digital clock struct
*/
struct self_digital_clock_info {
	u8 SC_OFF_SEL;
	u8 SC_TIME_UPDATE;
	u8 SC_24H_EN;
	u8 SC_SET_HH;
	u8 SC_SET_MM;
	u8 SC_SET_SS;
	u8 SC_SET_MSS;
	u8 SC_D_EN_HH;
	u8 SC_D_EN_MM;
	u8 SC_D_MIN_LOCK_EN;
	u32 SC_D_ST_HH_X10;
	u32 SC_D_ST_HH_Y10;
	u32 SC_D_ST_HH_X01;
	u32 SC_D_ST_HH_Y01;
	u32 SC_D_ST_MM_X10;
	u32 SC_D_ST_MM_Y10;
	u32 SC_D_ST_MM_X01;
	u32 SC_D_ST_MM_Y01;
	u32 SC_D_IMG_WIDTH;
	u32 SC_D_IMG_HEIGHT;
	u8 SD_BLC1_EN;
	u8 SD_BLC2_EN;
	u8 SD_BLEND_RATE;
	u8 SD_LINE_WIDTH;
	u32 SD_LINE_COLOR;
	u32 SD_LINE2_COLOR;
	u8 SD_RADIOUS1;
	u8 SD_RADIOUS2;
	u32 SD_BB_PT_X1;
	u32 SD_BB_PT_Y1;
	u32 SD_BB_PT_X2;
	u32 SD_BB_PT_Y2;
};

 /*
 * self blinking struct
 */
 struct self_blinking_info {
 	u8 SD_BLC1_EN;
	u8 SD_BLC2_EN;
	u8 SD_BLEND_RATE;
	u8 SD_LINE_WIDTH;
	u32 SD_LINE_COLOR;
	u32 SD_LINE2_COLOR;
	u8 SD_RADIUS1;
	u8 SD_RADIUS2;
	u32 SD_BB_PT_X1;
	u32 SD_BB_PT_Y1;
	u32 SD_BB_PT_X2;
	u32 SD_BB_PT_Y2;
 };
 #endif
