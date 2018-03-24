/*
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _SX9320_MULTI2_I2C_REG_H_
#define _SX9320_MULTI2_I2C_REG_H_

/*
 *  I2C Registers
 */
enum registers1 {
SX9320_MULTI2_IRQSTAT_REG = 0x00,
SX9320_MULTI2_STAT0_REG,
SX9320_MULTI2_STAT1_REG,
SX9320_MULTI2_STAT2_REG,
SX9320_MULTI2_STAT3_REG,
SX9320_MULTI2_IRQ_ENABLE_REG,
SX9320_MULTI2_IRQCFG0_REG,
SX9320_MULTI2_IRQCFG1_REG,
/* General Control */
SX9320_MULTI2_GNRLCTRL0_REG = 0x10,
SX9320_MULTI2_GNRLCTRL1_REG,
SX9320_MULTI2_I2CADDR_REG = 0x14,
/* Analog-Front-End (AFE) Control */
SX9320_MULTI2_AFECTRL0_REG = 0x20,
SX9320_MULTI2_AFECTRL1_REG, /*reserved*/
SX9320_MULTI2_AFECTRL2_REG, /*reseved*/
SX9320_MULTI2_AFECTRL3_REG,
SX9320_MULTI2_AFECTRL4_REG,
SX9320_MULTI2_AFECTRL5_REG, /*reserved*/
SX9320_MULTI2_AFECTRL6_REG,
SX9320_MULTI2_AFECTRL7_REG,
SX9320_MULTI2_AFEPH0_REG,
SX9320_MULTI2_AFEPH1_REG,
SX9320_MULTI2_AFEPH2_REG,
SX9320_MULTI2_AFEPH3_REG,
SX9320_MULTI2_AFECTRL8_REG,
SX9320_MULTI2_AFECTRL9_REG,
/* Main Digital Processing (Prox) Control */
SX9320_MULTI2_PROXCTRL0_REG = 0x30, /* 0~2 : rawfilt 3~5 : gain */
SX9320_MULTI2_PROXCTRL1_REG,
SX9320_MULTI2_PROXCTRL2_REG,
SX9320_MULTI2_PROXCTRL3_REG,
SX9320_MULTI2_PROXCTRL4_REG,
SX9320_MULTI2_PROXCTRL5_REG,
SX9320_MULTI2_PROXCTRL6_REG,
SX9320_MULTI2_PROXCTRL7_REG,
/* Advanced Digital Processing Control */
SX9320_MULTI2_ADVCTRL0_REG = 0x40,
SX9320_MULTI2_ADVCTRL1_REG,
SX9320_MULTI2_ADVCTRL2_REG,
SX9320_MULTI2_ADVCTRL3_REG,
SX9320_MULTI2_ADVCTRL4_REG,
SX9320_MULTI2_ADVCTRL5_REG,
SX9320_MULTI2_ADVCTRL6_REG,
SX9320_MULTI2_ADVCTRL7_REG,
SX9320_MULTI2_ADVCTRL8_REG,
SX9320_MULTI2_ADVCTRL9_REG,
SX9320_MULTI2_ADVCTRL10_REG, /* 0 ~ 3 : THD ph2, ph3 | 4 ~ 7 : THD ph0, ph1 */
SX9320_MULTI2_ADVCTRL11_REG,
SX9320_MULTI2_ADVCTRL12_REG,
SX9320_MULTI2_ADVCTRL13_REG, /* hysteresis */
SX9320_MULTI2_ADVCTRL14_REG,
SX9320_MULTI2_ADVCTRL15_REG,
SX9320_MULTI2_ADVCTRL16_REG,
SX9320_MULTI2_ADVCTRL17_REG,
SX9320_MULTI2_ADVCTRL18_REG,
SX9320_MULTI2_ADVCTRL19_REG,
SX9320_MULTI2_ADVCTRL20_REG,
/* Sensor Data Readback */
SX9320_MULTI2_REGSENSORSELECT = 0x60,
SX9320_MULTI2_REGUSEMSB,
SX9320_MULTI2_REGUSELSB,
SX9320_MULTI2_REGAVGMSB,
SX9320_MULTI2_REGAVGLSB,
SX9320_MULTI2_REGDIFFMSB,
SX9320_MULTI2_REGDIFFLSB,
SX9320_MULTI2_REGOFFSETMSB,
SX9320_MULTI2_REGOFFSETLSB,
SX9320_MULTI2_SARMSB,
SX9320_MULTI2_SARLSB,
/* Miscellaneous */
SX9320_MULTI2_SOFTRESET_REG = 0x9F,
SX9320_MULTI2_WHOAMI_REG = 0xFA,
SX9320_MULTI2_REV_REG = 0xFE,
};

/* IrqStat 0:Inactive 1:Active */
#define SX9320_MULTI2_IRQSTAT_RESET_FLAG	0x80
#define SX9320_MULTI2_IRQSTAT_TOUCH_FLAG	0x40
#define SX9320_MULTI2_IRQSTAT_RELEASE_FLAG	0x20
#define SX9320_MULTI2_IRQSTAT_COMPDONE_FLAG	0x10
#define SX9320_MULTI2_IRQSTAT_CONV_FLAG	0x08
#define SX9320_MULTI2_IRQSTAT_PROG2_FLAG	0x04
#define SX9320_MULTI2_IRQSTAT_PROG1_FLAG	0x02
#define SX9320_MULTI2_IRQSTAT_PROG0_FLAG	0x01

/* CpsStat */
#define SX9320_MULTI2_STAT0_PROXSTAT_PH3_FLAG  0x08
#define SX9320_MULTI2_STAT0_PROXSTAT_PH2_FLAG  0x04
#define SX9320_MULTI2_STAT0_PROXSTAT_PH1_FLAG  0x02
#define SX9320_MULTI2_STAT0_PROXSTAT_PH0_FLAG  0x01

/* SoftReset */
#define SX9320_MULTI2_SOFTRESET  0xDE

/* Manual Compensation */
#define SX9320_MULTI2_STAT2_COMPSTAT_PH3_FLAG 0x08
#define SX9320_MULTI2_STAT2_COMPSTAT_PH2_FLAG 0x04
#define SX9320_MULTI2_STAT2_COMPSTAT_PH1_FLAG 0x02
#define SX9320_MULTI2_STAT2_COMPSTAT_PH0_FLAG 0x01
#define SX9320_MULTI2_STAT2_COMPSTAT_ALL_FLAG (SX9320_MULTI2_STAT2_COMPSTAT_PH3_FLAG | \
					SX9320_MULTI2_STAT2_COMPSTAT_PH2_FLAG | \
					SX9320_MULTI2_STAT2_COMPSTAT_PH1_FLAG | \
					SX9320_MULTI2_STAT2_COMPSTAT_PH0_FLAG)

#define SX9320_MULTI2_SMALL_RANGE_VALUE	26500
#define SX9320_MULTI2_LARGE_RANGE_VALUE	53000

/* for device tree parse */
#define SX9320_MULTI2_PHEN		"sx9320_multi2,phen"
#define SX9320_MULTI2_GAIN		"sx9320_multi2,gain"
#define SX9320_MULTI2_AGAIN		"sx9320_multi2,again"
#define SX9320_MULTI2_SCANPERIOD	"sx9320_multi2,scan_period"
#define SX9320_MULTI2_RANGE		"sx9320_multi2,range"
#define SX9320_MULTI2_SAMPLING_FREQ	"sx9320_multi2,sampling_freq"
#define SX9320_MULTI2_RESOLUTION	"sx9320_multi2,resolution"
#define SX9320_MULTI2_RAWFILT		"sx9320_multi2,rawfilt"
#define SX9320_MULTI2_HYST		"sx9320_multi2,hyst"
#define SX9320_MULTI2_AVGPOSFILT	"sx9320_multi2,avgposfilt"
#define SX9320_MULTI2_AVGNEGFILT	"sx9320_multi2,avgnegfilt"
#define SX9320_MULTI2_AVGTHRESH		"sx9320_multi2,avgthresh"
#define SX9320_MULTI2_DEBOUNCER		"sx9320_multi2,debouncer"
#define SX9320_MULTI2_NORMALTHD		"sx9320_multi2,normal_thd"
#define SX9320_MULTI2_AFEPH0		"sx9320_multi2,afeph0"
#define SX9320_MULTI2_AFEPH1		"sx9320_multi2,afeph1"
#define SX9320_MULTI2_AFEPH2		"sx9320_multi2,afeph2"
#define SX9320_MULTI2_AFEPH3		"sx9320_multi2,afeph3"

#define SX9320_MULTI2_CS2_GND
//#define SX9320_MULTI2_CS0_GND

struct smtc_reg_data {
	unsigned char reg;
	unsigned char val;
};

#define SX9320_MULTI2_PHEN_SELECT 0x02 /* PH3, PH2, PH1, PH0 : 4bits*/
/*
 * this struct sets each register as a default in case of
 * scan_period, phase, analog gain, sampling_freq, raw_filt, gain, analog_gain,
 * avgposfilt, avgnegfilt, avgthresh, threshold, hysteresis, range.
 */

static struct smtc_reg_data setup_reg[] = {
{
	.reg = SX9320_MULTI2_GNRLCTRL0_REG, /* 0 - SCANPERIOD */
	.val = 0x16, /* [ph 0,1 scan period : 100ms] */
},
{
	.reg = SX9320_MULTI2_IRQ_ENABLE_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_GNRLCTRL1_REG, /* 2 - PHEN */
	.val = 0x20, /* [PHEN all off] [ph 2,3 scan period : 100ms] */
},
{
	.reg = SX9320_MULTI2_AFECTRL0_REG, /* 3 */
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_AFECTRL3_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_AFECTRL4_REG, /* 5 - Sampling freq ph 0/1 */
	.val = 0x44, /* [sampling req : 83.33kHz]*/
},
{
	.reg = SX9320_MULTI2_AFECTRL6_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_AFECTRL7_REG, /* 7 - Sampling freq ph 2/3 */
	.val = 0x44, /* [sampling req : 83.33kHz]*/
},
    /* set the phases to be similar to the previous gen */
    /* SX9320_MULTI2_AFEPH0~3_REGs : defines the CS pins usage during phase 0~3 */
{
	.reg = SX9320_MULTI2_AFEPH0_REG,
	.val = 0x2a,
},
{
	.reg = SX9320_MULTI2_AFEPH1_REG,
	.val = 0x2a,
},
{
	.reg = SX9320_MULTI2_AFEPH2_REG,
	.val = 0x2a,
},
{
	.reg = SX9320_MULTI2_AFEPH3_REG,
	.val = 0x26,
},
{
	.reg = SX9320_MULTI2_PROXCTRL0_REG, /* 12 - Rawfilt & Gain ph 0/1 */
	.val = 0x09, /* [GAIN : x1] [RAWFILT : 1-1/2] */
},
{
	.reg = SX9320_MULTI2_PROXCTRL1_REG, /* 13 - Rawfilt & Gain ph 2/3 */
	.val = 0x09, /* [GAIN : x1] [RAWFILT : 1-1/2] */
},
{
	.reg = SX9320_MULTI2_PROXCTRL2_REG, /* 14 - Avgthresh */
	.val = 0x20, /* [AVGTHRESH : 16384] */
},
{
	.reg = SX9320_MULTI2_PROXCTRL3_REG,
	.val = 0x20,
},
{
	.reg = SX9320_MULTI2_PROXCTRL4_REG, /* 16 - AVGposfilt & avgnegfilt */
	.val = 0x0C, /* [avgnegfilt : 1-1/2 avgposfilt : 1-1/256] */
},
{
	.reg = SX9320_MULTI2_PROXCTRL5_REG, /* 17 - HYST */
	.val = 0x00, /* [Hysteresis : None] */
},
{
	.reg = SX9320_MULTI2_PROXCTRL6_REG, /* 18 - threshold ph 0/1 */
	.val = 0x08, /* 32 */
},
{
	.reg = SX9320_MULTI2_PROXCTRL7_REG, /* 19 - threshold ph 2/3 */
	.val = 0x08, /* 32 */
},
{
	.reg = SX9320_MULTI2_ADVCTRL0_REG, //20
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL1_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL2_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL3_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL4_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL5_REG,
	.val = 0x05,
},
{
	.reg = SX9320_MULTI2_ADVCTRL6_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL7_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL8_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL9_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL10_REG,// 30
	.val = 0x40,
},
{
	.reg = SX9320_MULTI2_ADVCTRL11_REG,
	.val = 0x21,
},
{
	.reg = SX9320_MULTI2_ADVCTRL12_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL13_REG,
	.val = 0x04,
},
{
	.reg = SX9320_MULTI2_ADVCTRL14_REG,
	.val = 0x80,
},
{
	.reg = SX9320_MULTI2_ADVCTRL15_REG,
	.val = 0x0C,
},
{
	.reg = SX9320_MULTI2_ADVCTRL16_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL17_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL18_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL19_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_ADVCTRL20_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_IRQCFG0_REG,
	.val = 0x00,
},
{
	.reg = SX9320_MULTI2_IRQCFG1_REG, /* 42 */
	.val = 0x80,
},
{
	.reg = SX9320_MULTI2_AFECTRL9_REG,
	.val = 0x08,
},
};

enum {
	OFF = 0,
	ON = 1
};

extern int sensors_create_symlink(struct input_dev *inputdev);
extern void sensors_remove_symlink(struct input_dev *inputdev);
extern int sensors_register(struct device *dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
extern void sensors_unregister(struct device *dev,
	struct device_attribute *attributes[]);

#endif /* _SX9320_MULTI2_I2C_REG_H_*/
