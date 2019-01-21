
#include <linux/io.h>
#include <linux/module.h>
//#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
//#include <linux/interrupt.h>
//#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/exynos_ion.h>

#include "fimc-is-vender-config.h"


#include "fimc-is-binary.h"
#include "fimc-is-mem.h"

#define LEN_OF_VECTOR_SET 17
#define NUM_VECTOR_SET 2

static unsigned int get_newline_offset(const char *src, size_t size)
{
	size_t offset = 1;

	do {
		offset++;
		if ((*src == 0x0d) && (*(src + 1) == 0x0a))
			return offset;
		src++;
	} while (offset <= size);

	return 0;
}


static int reset_value_test(char *ip)
{
	struct fimc_is_binary bin;
	char *filename;
	unsigned int addr, val, val_read;
	unsigned int ofs;
	int ret;
	size_t count = 0;
	char *buf;

	void __iomem *baseaddr;

	filename = __getname();
	if (unlikely(!filename))
		return -ENOMEM;

	snprintf(filename, PATH_MAX, "sfrtest/%s_reset%s", ip, ".txt");

	ret = request_binary(&bin, FIMC_IS_ISP_LIB_SDCARD_PATH, filename, NULL);
	if (ret) {
		err_vec("failed to load sfr reset configuration (%d): %sname: %s",
				ret, FIMC_IS_ISP_LIB_SDCARD_PATH, filename);
		__putname(filename);
		return ret;
	} else {
		info_vec("sfr reset configuration info - size: 0x%lx name: %s\n",
				bin.size, filename);
		__putname(filename);
	}

	baseaddr = ioremap_nocache(IO_MEM_BASE, IO_MEM_SIZE);

	buf = (char *)bin.data;
	while (count < bin.size) {
		if (sscanf(buf, "%x %x\n", &addr, &val) == NUM_VECTOR_SET) {
			dbg_psv("addr: 0x%08x, val: 0x%08x\n", addr, val);
			count += LEN_OF_VECTOR_SET;
			buf += LEN_OF_VECTOR_SET;

			val_read = __raw_readl(baseaddr + (addr - IO_MEM_VECTOR_OFS));

			if (val != val_read) {
				ret++;
				err_vec("reset Mis-match found at 0x%08x: 0x%08x != 0x%08x\n", addr, val, val_read);
				}

		}

		ofs = get_newline_offset(buf, bin.size - count);
		if (ofs) {
			count += ofs;
			buf += ofs;
		} else {
			break;
		}
	}

	iounmap(baseaddr);

	release_binary(&bin);

	info_vec("%s finished\n", ip);
	return ret;
}

static int sfr_rw_test(char *ip)
{
	struct fimc_is_binary bin;
	char *filename;
	unsigned int addr, val, val_read;
	unsigned int ofs;
	int ret;
	size_t count = 0;
	char *buf;

	void __iomem *baseaddr;

	filename = __getname();
	if (unlikely(!filename))
		return -ENOMEM;

	snprintf(filename, PATH_MAX, "sfrtest/%s_access%s", ip, ".txt");

	ret = request_binary(&bin, FIMC_IS_ISP_LIB_SDCARD_PATH, filename, NULL);
	if (ret) {
		err_vec("failed to load sfr RW configuration (%d): %sname: %s",
				ret, FIMC_IS_ISP_LIB_SDCARD_PATH, filename);
		__putname(filename);
		return ret;
	} else {
		info_vec("sfr RW configuration info - size: 0x%lx name: %s\n",
				bin.size, filename);
		__putname(filename);
	}

	baseaddr = ioremap_nocache(IO_MEM_BASE, IO_MEM_SIZE);

	buf = (char *)bin.data;
	while (count < bin.size) {
		if (sscanf(buf, "%x %x\n", &addr, &val) == NUM_VECTOR_SET) {
			dbg_psv("addr: 0x%08x, val: 0x%08x\n", addr, val);
			count += LEN_OF_VECTOR_SET;
			buf += LEN_OF_VECTOR_SET;

			 __raw_writel(val, baseaddr + (addr -  IO_MEM_VECTOR_OFS));
			val_read = __raw_readl(baseaddr + (addr - IO_MEM_VECTOR_OFS));

			if (val != val_read) {
				ret++;
				err_vec("R/W Mis-match at 0x%08x: 0x%08x != 0x%08x\n", addr, val, val_read);
				}

		}

		ofs = get_newline_offset(buf, bin.size - count);
		if (ofs) {
			count += ofs;
			buf += ofs;
		} else {
			break;
		}
	}

	iounmap(baseaddr);

	release_binary(&bin);

	return ret;
}

static int reset_value_test_no_fimc_is(char *ip, unsigned int phy_baseaddr)
{
	struct fimc_is_binary bin;
	char *filename;
	unsigned int addr, val, val_read;
	unsigned int ofs;
	int ret;
	size_t count = 0;
	char *buf;

	void __iomem *baseaddr;

	filename = __getname();
	if (unlikely(!filename))
		return -ENOMEM;

	snprintf(filename, PATH_MAX, "sfrtest/%s_reset%s", ip, ".txt");

	ret = request_binary(&bin, FIMC_IS_ISP_LIB_SDCARD_PATH, filename, NULL);
	if (ret) {
		err_sfr("failed to load sfr reset configuration (%d): %sname: %s",
				ret, FIMC_IS_ISP_LIB_SDCARD_PATH, filename);
		__putname(filename);
		return ret;
	} else {
		info_sfr("sfr reset configuration info - size: 0x%lx name: %s\n",
				bin.size, filename);
		__putname(filename);
	}

	baseaddr = ioremap_nocache(phy_baseaddr, 0x10000);

	buf = (char *)bin.data;
	while (count < bin.size) {
		if (sscanf(buf, "%x %x\n", &addr, &val) == NUM_VECTOR_SET) {
			dbg_sfr("addr: 0x%08x, val: 0x%08x\n", addr, val);
			count += LEN_OF_VECTOR_SET;
			buf += LEN_OF_VECTOR_SET;

			val_read = __raw_readl(baseaddr + addr);

			if (val != val_read) {
				ret++;
				err_sfr("reset mis-match found at 0x%08x: 0x%08x != 0x%08x\n", phy_baseaddr + addr, val, val_read);
				}

		}

		ofs = get_newline_offset(buf, bin.size - count);
		if (ofs) {
			count += ofs;
			buf += ofs;
		} else {
			break;
		}
	}

	iounmap(baseaddr);

	release_binary(&bin);

	info_sfr("%s finished\n", ip);
	return ret;
}

static int sfr_rw_test_no_fimc_is(char *ip, unsigned int phy_baseaddr)
{
	struct fimc_is_binary bin;
	char *filename;
	unsigned int addr, val, val_read;
	unsigned int ofs;
	int ret;
	size_t count = 0;
	char *buf;

	void __iomem *baseaddr;

	filename = __getname();
	if (unlikely(!filename))
		return -ENOMEM;

	snprintf(filename, PATH_MAX, "sfrtest/%s_access%s", ip, ".txt");

	ret = request_binary(&bin, FIMC_IS_ISP_LIB_SDCARD_PATH, filename, NULL);
	if (ret) {
		err_sfr("failed to load sfr RW configuration (%d): %sname: %s",
				ret, FIMC_IS_ISP_LIB_SDCARD_PATH, filename);
		__putname(filename);
		return ret;
	} else {
		info_sfr("sfr RW configuration info - size: 0x%lx name: %s\n",
				bin.size, filename);
		__putname(filename);
	}

	baseaddr = ioremap_nocache(phy_baseaddr, 0x10000);

	buf = (char *)bin.data;
	while (count < bin.size) {
		if (sscanf(buf, "%x %x\n", &addr, &val) == NUM_VECTOR_SET) {
			dbg_sfr("addr: 0x%08x, val: 0x%08x\n", addr, val);
			count += LEN_OF_VECTOR_SET;
			buf += LEN_OF_VECTOR_SET;

			 __raw_writel(val, baseaddr + addr);
			val_read = __raw_readl(baseaddr + addr);

			if (val != val_read) {
				ret++;
				err_sfr("R/W Mis-match at 0x%08x: 0x%08x != 0x%08x\n", phy_baseaddr + addr, val, val_read);
				}

		}

		ofs = get_newline_offset(buf, bin.size - count);
		if (ofs) {
			count += ofs;
			buf += ofs;
		} else {
			break;
		}
	}

	iounmap(baseaddr);

	release_binary(&bin);

	return ret;
}

int csis0_rstval_test(void)
{
	return reset_value_test("csis0");
}

int csis1_rstval_test(void)
{
	return reset_value_test("csis1");
}

int bns_rstval_test(void)
{
	return reset_value_test("bns");
}

int taa_rstval_test(void)
{
	return reset_value_test("taa");
}

int isp_rstval_test(void)
{
	return reset_value_test("isp");
}

int mcsc_rstval_test(void)
{
	return reset_value_test("mcsc");
}

int vra_rstval_test(void)
{
	return reset_value_test("vra");
}

int mfcmcsc_rstval_test(void)
{
	return reset_value_test_no_fimc_is("mfcmcsc", 0x12C00000);
}

int csis0_sfr_access_test(void)
{
	return sfr_rw_test("csis0");
}

int csis1_sfr_access_test(void)
{
	return sfr_rw_test("csis1");
}

int bns_sfr_access_test(void)
{
	return sfr_rw_test("bns");
}

int taa_sfr_access_test(void)
{
	return sfr_rw_test("taa");
}

int isp_sfr_access_test(void)
{
	return sfr_rw_test("isp");
}

int mcsc_sfr_access_test(void)
{
	return sfr_rw_test("mcsc");
}

int vra_sfr_access_test(void)
{
	return sfr_rw_test("vra");
}

int mfcmcsc_sfr_access_test(void)
{
	return sfr_rw_test_no_fimc_is("mfcmcsc", 0x12c00000);
}

#if defined(CONFIG_SOC_EXYNOS8895)
static int fimc_is_reset_8895(void)
{
	void __iomem *baseaddr;
	u32 readval;
	int sfr_offs = 0;

	/* csis0 */
	baseaddr = ioremap(0x12CA0000, 0x10000);
	__raw_writel(0x2, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if ((readval & 0x2) == 0x0)
			break;
	}
	iounmap(baseaddr);
	info_vec("csis0 reset...\n");

	/* csis1 */
	baseaddr = ioremap(0x12CB0000, 0x10000);
	__raw_writel(0x2, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if ((readval & 0x2) == 0x0)
			break;
	}
	iounmap(baseaddr);
	info_vec("csis1 reset...\n");

	/* csis2 */
	baseaddr = ioremap(0x12CC0000, 0x10000);
	__raw_writel(0x2, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if ((readval & 0x2) == 0x0)
			break;
	}
	iounmap(baseaddr);
	info_vec("csis2 reset...\n");

	/* csis3 */
	baseaddr = ioremap(0x12CD0000, 0x10000);
	__raw_writel(0x2, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if ((readval & 0x2) == 0x0)
			break;
	}
	iounmap(baseaddr);
	info_vec("csis3 reset...\n");

	/* csis-dma */
	baseaddr = ioremap(0x12CE0000, 0x10000);
	__raw_writel(0x1, baseaddr + 0x8);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if ((readval & 0x1) == 0x0)
			break;
	}
	iounmap(baseaddr);
	info_vec("csis-dma reset...\n");

	/* bns */
	baseaddr = ioremap(0x12C80000, 0x10000);
	__raw_writel(0x00020000, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if (readval == 0)
			break;
	}
	iounmap(baseaddr);
	info_vec("bns reset...\n");

	/* 3aaw */
	baseaddr = ioremap(0x13030000, 0x10000);
	while (1) {
		readval = __raw_readl(baseaddr + 0x20);
		if (readval  == 0x00000001)
			break;
	}
	__raw_writel(0x00000001, baseaddr + 0xC);
	while (1) {
		readval = __raw_readl(baseaddr + 0xC);
		if (readval  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_vec("3aaw reset...\n");

	/* 3aa */
	baseaddr = ioremap(0x13130000, 0x10000);
	while (1) {
		readval = __raw_readl(baseaddr + 0x20);
		if (readval  == 0x00000001)
			break;
	}
	__raw_writel(0x00000001, baseaddr + 0xC);
	while (1) {
		readval = __raw_readl(baseaddr + 0xC);
		if (readval  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_vec("3aa reset...\n");

	/* isplp */
	baseaddr = ioremap(0x13040000, 0x10000);
	while (1) {
		readval = __raw_readl(baseaddr + 0x20);
		if (readval  == 0x00000001)
			break;
	}
	__raw_writel(0x00000001, baseaddr + 0xC);
	while (1) {
		readval = __raw_readl(baseaddr + 0xC);
		if (readval  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_vec("isplp reset...\n");

	/* isphq */
	baseaddr = ioremap(0x13140000, 0x10000);
	while (1) {
		readval = __raw_readl(baseaddr + 0x20);
		if (readval  == 0x00000001)
			break;
	}
	__raw_writel(0x00000001, baseaddr + 0xC);
	while (1) {
		readval = __raw_readl(baseaddr + 0xC);
		if (readval  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_vec("isphq reset...\n");

	/* dcp */
	baseaddr = ioremap(0x12F00000, 0x10000);
	__raw_writel(0x00000001, baseaddr + 0xC);
	while (1) {
		readval = __raw_readl(baseaddr + 0xC);
		if (readval  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_vec("dcp reset...\n");

	/* srdz */
	baseaddr = ioremap(0x14300000, 0x10000);

	__raw_writel(0x00000003, baseaddr + 0x18);	//DMA_FLUSH_REQ
	while (1) {
		readval = __raw_readl(baseaddr + 0x1C);
		if (readval  == 0x00000000)
			break;
	}

	__raw_writel(0x00000001, baseaddr + 0x10);	//sw_reset
	while (1) {
		readval = __raw_readl(baseaddr + 0x14);
		if (readval  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_vec("srdz reset...\n");

	/* tpu0 */
	baseaddr = ioremap(0x12C40000, 0x10000);
	while (1) {
		readval = __raw_readl(baseaddr + 0x20);
		if (readval  == 0x00000001)
			break;
	}
	__raw_writel(0x00000001, baseaddr + 0xC);
	while (1) {
		readval = __raw_readl(baseaddr + 0xC);
		if (readval  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_vec("tpu0 reset...\n");

	/* tpu1 */
	baseaddr = ioremap(0x12C90000, 0x10000);
	while (1) {
		readval = __raw_readl(baseaddr + 0x20);
		if (readval  == 0x00000001)
			break;
	}
	__raw_writel(0x00000001, baseaddr + 0xC);
	while (1) {
		readval = __raw_readl(baseaddr + 0xC);
		if (readval  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_vec("tpu1 reset...\n");

	/* mcsc */
	baseaddr = ioremap(0x12C50000, 0x10000);
/*	__raw_writel(0x00000000, baseaddr + 0x0);
	__raw_writel(0x00000000, baseaddr + 0x07a4);
	__raw_writel(0xFFFFFFFF, baseaddr + 0x07ac);
	__raw_writel(0x00000001, baseaddr + 0x24);
	__raw_writel(0x00000001, baseaddr + 0x800);
	__raw_writel(0x00000001, baseaddr + 0x20);
	*/
	__raw_writel(0x00000001, baseaddr + 0x20);
	__raw_writel(0x00000001, baseaddr + 0x24);
	__raw_writel(0x00000001, baseaddr + 0x28);

	while (1) {
		readval = __raw_readl(baseaddr + 0x790);
		if ((readval & 1)  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_vec("mcsc reset...\n");

	/* vra */
	baseaddr = ioremap(0x12C60000, 0x10000);
	__raw_writel(0x00000001, baseaddr + 0x3008);
	while (1) {
		readval = __raw_readl(baseaddr + 0x300C);
		if ((readval & 1)  == 0x00000001)
			break;
		}
	__raw_writel(0x00000001, baseaddr + 0x3004);

	__raw_writel(0x00000002, baseaddr + 0xb04C);
	__raw_writel(0x00000001, baseaddr + 0xb008);
	while (1) {
		readval = __raw_readl(baseaddr + 0xb00C);
		if ((readval & 1)  == 0x00000001)
			break;
		}
	__raw_writel(0x00000001, baseaddr + 0xb004);
	__raw_writel(0x00000001, baseaddr + 0xb048);

	/* internal sram clear (instruction memory) */
	for (sfr_offs = 0; sfr_offs < 2048; sfr_offs++)
		__raw_writel(0x00000000, (baseaddr + 0xE000 + sfr_offs * 4));

	iounmap(baseaddr);
	info_vec("vra reset...\n");

	return 0;
}

int fimc_is_sfr_test_8895(void)
{
	int errorcode = 0;

	info_sfr("fimc_is_sfr_test start\n");

	fimc_is_reset_8895();
	errorcode = errorcode | reset_value_test("csis0");
	errorcode = errorcode | reset_value_test("csis1");
	errorcode = errorcode | reset_value_test("csis2");
	errorcode = errorcode | reset_value_test("csis3");
	errorcode = errorcode | reset_value_test("csisx4_dma");
	errorcode = errorcode | reset_value_test("bns");
	errorcode = errorcode | reset_value_test("taa0");
	errorcode = errorcode | reset_value_test("taa1");
	errorcode = errorcode | reset_value_test("isp0");
	errorcode = errorcode | reset_value_test("isp1");
	errorcode = errorcode | reset_value_test("tpu0");
	errorcode = errorcode | reset_value_test("tpu1");
	errorcode = errorcode | reset_value_test("mcsc");
	errorcode = errorcode | reset_value_test("vra");
	errorcode = errorcode | reset_value_test("dcp");
	errorcode = errorcode | reset_value_test("srdz");

	errorcode = errorcode | sfr_rw_test("csis0");
	errorcode = errorcode | sfr_rw_test("csis1");
	errorcode = errorcode | sfr_rw_test("csis2");
	errorcode = errorcode | sfr_rw_test("csis3");
	errorcode = errorcode | sfr_rw_test("csisx4_dma");
	errorcode = errorcode | sfr_rw_test("bns");
	errorcode = errorcode | sfr_rw_test("taa0");
	errorcode = errorcode | sfr_rw_test("taa1");
	errorcode = errorcode | sfr_rw_test("isp0");
	errorcode = errorcode | sfr_rw_test("isp1");
	errorcode = errorcode | sfr_rw_test("tpu0");
	errorcode = errorcode | sfr_rw_test("tpu1");
	errorcode = errorcode | sfr_rw_test("mcsc");
	errorcode = errorcode | sfr_rw_test("vra");
	errorcode = errorcode | sfr_rw_test("dcp");
	errorcode = errorcode | sfr_rw_test("srdz");

	fimc_is_reset_8895();
	errorcode = errorcode | reset_value_test("csis0");
	errorcode = errorcode | reset_value_test("csis1");
	errorcode = errorcode | reset_value_test("csis2");
	errorcode = errorcode | reset_value_test("csis3");
	errorcode = errorcode | reset_value_test("csisx4_dma");
	errorcode = errorcode | reset_value_test("bns");
	errorcode = errorcode | reset_value_test("taa0");
	errorcode = errorcode | reset_value_test("taa1");
	errorcode = errorcode | reset_value_test("isp0");
	errorcode = errorcode | reset_value_test("isp1");
	errorcode = errorcode | reset_value_test("tpu0");
	errorcode = errorcode | reset_value_test("tpu1");
	errorcode = errorcode | reset_value_test("mcsc");
	errorcode = errorcode | reset_value_test("vra");
	errorcode = errorcode | reset_value_test("dcp");
	errorcode = errorcode | reset_value_test("srdz");

	info_sfr("fimc_is_sfr_test uErrorCode: 0x%x\n", errorcode);

	return errorcode;
}
#elif defined(CONFIG_SOC_EXYNOS7880)
static int fimc_is_reset_7880(void)
{
	void __iomem *baseaddr;
	u32 readval;

	/* csis0 */
	baseaddr = ioremap(0x14420000, 0x10000);
	__raw_writel(0x2, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if (readval == 0x00004000)
			break;
		}
	iounmap(baseaddr);
	info_sfr("csis0 reset...\n");

	/* csis1 */
	baseaddr = ioremap(0x14460000, 0x10000);
	__raw_writel(0x2, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if (readval == 0x00004000)
			break;
		}
	iounmap(baseaddr);
	info_sfr("csis1 reset...\n");

	/* bns */
	baseaddr = ioremap(0x14410000, 0x10000);
	__raw_writel(0x00080000, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if ((readval & (1<<18)) == (1<<18))
			break;
		}
	__raw_writel(0x00020000, baseaddr + 0x4);
	iounmap(baseaddr);
	info_sfr("bns reset...\n");

	/* 3aa */
	baseaddr = ioremap(0x14480000, 0x10000);
	__raw_writel(0x00000001, baseaddr + 0xC);
	while (1) {
		readval = __raw_readl(baseaddr + 0xC);
		if (readval  == 0x00000000)
			break;
		}
	iounmap(baseaddr);
	info_sfr("3aa reset...\n");

	/* isp */
	baseaddr = ioremap(0x14400000, 0x10000);
	__raw_writel(0x00000001, baseaddr + 0xC);
	while (1) {
		readval = __raw_readl(baseaddr + 0xC);
		if (readval  == 0x00000000)
			break;
		}
	iounmap(baseaddr);
	info_sfr("isp reset...\n");

	/* mcsc */
	baseaddr = ioremap(0x14430000, 0x10000);
	__raw_writel(0x00000000, baseaddr + 0x0);
	__raw_writel(0x00000000, baseaddr + 0x07a4);
	__raw_writel(0xFFFFFFFF, baseaddr + 0x07ac);
	__raw_writel(0x00000001, baseaddr + 0x24);
	__raw_writel(0x00000001, baseaddr + 0x800);
	__raw_writel(0x00000001, baseaddr + 0x20);
	while (1) {
		readval = __raw_readl(baseaddr + 0x790);
		if ((readval & 1)  == 0x00000000)
			break;
}
	iounmap(baseaddr);
	info_sfr("mcsc reset...\n");

	/* vra */
	baseaddr = ioremap(0x14440000, 0x10000);
	__raw_writel(0x00000001, baseaddr + 0x3008);
	while (1) {
		readval = __raw_readl(baseaddr + 0x300C);
		if ((readval & 1)  == 0x00000001)
			break;
		}
	__raw_writel(0x00000001, baseaddr + 0x3004);

	__raw_writel(0x00000002, baseaddr + 0xb04C);
	__raw_writel(0x00000001, baseaddr + 0xb008);
	while (1) {
		readval = __raw_readl(baseaddr + 0xb00C);
		if ((readval & 1)  == 0x00000001)
			break;
		}
	__raw_writel(0x00000001, baseaddr + 0xb004);
	__raw_writel(0x00000001, baseaddr + 0xb048);

	iounmap(baseaddr);
	info_sfr("vra reset...\n");

	return 0;
}

int fimc_is_sfr_test_7880(void)
{
	int errorcode = 0;

	info_sfr("fimc_is_sfr_test start\n");

	fimc_is_reset();
	errorcode = errorcode | csis0_rstval_test();
	errorcode = errorcode | csis1_rstval_test();
	errorcode = errorcode | bns_rstval_test();
	errorcode = errorcode | taa_rstval_test();
	errorcode = errorcode | isp_rstval_test();
	errorcode = errorcode | mcsc_rstval_test();
	errorcode = errorcode | vra_rstval_test();

	errorcode = errorcode | csis0_sfr_access_test();
	errorcode = errorcode | csis1_sfr_access_test();
	errorcode = errorcode | bns_sfr_access_test();
	errorcode = errorcode | taa_sfr_access_test();
	errorcode = errorcode | isp_sfr_access_test();
	errorcode = errorcode | mcsc_sfr_access_test();
	errorcode = errorcode | vra_sfr_access_test();

	fimc_is_reset();

	errorcode = errorcode | csis0_rstval_test();
	errorcode = errorcode | csis1_rstval_test();
	errorcode = errorcode | bns_rstval_test();
	errorcode = errorcode | taa_rstval_test();
	errorcode = errorcode | isp_rstval_test();
	errorcode = errorcode | mcsc_rstval_test();
	errorcode = errorcode | vra_rstval_test();

	info_sfr("fimc_is_sfr_test uErrorCode: 0x%x\n", errorcode);

	return errorcode;
}

#elif defined(CONFIG_SOC_EXYNOS7570)
static int fimc_is_reset_7570(void)
{
	void __iomem *baseaddr;
	u32 readval;
	int sfr_offs = 0;

	/* csis0 */
	baseaddr = ioremap(0x14420000, 0x10000);
	__raw_writel(0x2, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if (readval == 0x00004000)
			break;
		}
	iounmap(baseaddr);
	info_sfr("csis0 reset...\n");

	/* bsc */
	baseaddr = ioremap(0x14410000, 0x10000);
	__raw_writel(0x00020000, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if (readval == 0x00040000)
			break;
		}
	iounmap(baseaddr);
	info_sfr("bsc reset...\n");

	/* isp : AXIDMA reset */
	baseaddr = ioremap(0x14400000, 0x10000);
	__raw_writel(0x00000002, baseaddr + 0x3200);
	while (1) {
		readval = __raw_readl(baseaddr + 0x3204);
		if (readval  == 0x00000004)
			break;
		}
	__raw_writel(0x00000001, baseaddr + 0x3208);
	iounmap(baseaddr);
	info_sfr("isp reset...\n");

	/* mcsc */
	baseaddr = ioremap(0x14430000, 0x10000);
	__raw_writel(0x00000000, baseaddr + 0x0);		//disable
	__raw_writel(0x00000000, baseaddr + 0x07a4);
	__raw_writel(0xFFFFFFFF, baseaddr + 0x07ac);
	__raw_writel(0x00000001, baseaddr + 0x20);
	while (1) {
		readval = __raw_readl(baseaddr + 0x790);
		if ((readval & 1)  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_sfr("mcsc reset...\n");

	/* vra */
	baseaddr = ioremap(0x14440000, 0x10000);
	__raw_writel(0x00000002, baseaddr + 0xb04C);
	__raw_writel(0x00000001, baseaddr + 0xb008);
	while (1) {
		readval = __raw_readl(baseaddr + 0xb00C);
		if ((readval & 1)  == 0x00000001)
			break;
		}
	__raw_writel(0x00000001, baseaddr + 0xb004);
	__raw_writel(0x00000001, baseaddr + 0xb048);

	/* internal sram clear (instruction memory) */
	for (sfr_offs = 0; sfr_offs < 2048; sfr_offs++)
		__raw_writel(0x00000000, (baseaddr + 0xE000 + sfr_offs * 4));

	iounmap(baseaddr);
	info_sfr("vra reset...\n");

	/* mfc_mcsc */
	baseaddr = ioremap(0x12c00000, 0x10000);
	__raw_writel(0x00000000, baseaddr + 0x0);		//disable
	__raw_writel(0x00000000, baseaddr + 0x07a4);
	__raw_writel(0xFFFFFFFF, baseaddr + 0x07ac);
	__raw_writel(0x00000001, baseaddr + 0x0800);	//HWFC swreset
	__raw_writel(0x00000001, baseaddr + 0x20);
	while (1) {
		readval = __raw_readl(baseaddr + 0x790);
		if ((readval & 1)  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_sfr("mcsc reset...\n");

	return 0;
}

int fimc_is_sfr_test_7570(void)
{
	int errorcode = 0;

	dbg_sfr("7570 fimc_is_sfr_test start\n");

	fimc_is_reset_7570();
	errorcode = errorcode | csis0_rstval_test();
	errorcode = errorcode | bns_rstval_test();
	errorcode = errorcode | isp_rstval_test();
	errorcode = errorcode | mcsc_rstval_test();
	errorcode = errorcode | vra_rstval_test();
	errorcode = errorcode | mfcmcsc_rstval_test();
	info_sfr("fimc_is_rstval_test uErrorCode: 0x%x\n", errorcode);

	errorcode = errorcode | csis0_sfr_access_test();
	errorcode = errorcode | bns_sfr_access_test();
	errorcode = errorcode | isp_sfr_access_test();
	errorcode = errorcode | mcsc_sfr_access_test();
	errorcode = errorcode | vra_sfr_access_test();
	errorcode = errorcode | mfcmcsc_sfr_access_test();
	info_sfr("fimc_is_access_test uErrorCode: 0x%x\n", errorcode);

	fimc_is_reset_7570();
	errorcode = errorcode | csis0_rstval_test();
	errorcode = errorcode | bns_rstval_test();
	errorcode = errorcode | isp_rstval_test();
	errorcode = errorcode | mcsc_rstval_test();
	errorcode = errorcode | vra_rstval_test();
	errorcode = errorcode | mfcmcsc_rstval_test();
	info_sfr("fimc_is_rstval2_test uErrorCode: 0x%x\n", errorcode);

	return errorcode;
}
#endif

static int fimc_is_reset(void)
{
	int errorcode = 0;

#if defined(CONFIG_SOC_EXYNOS8895)
	errorcode = fimc_is_reset_8895();
#elif defined(CONFIG_SOC_EXYNOS7880)
	errorcode = fimc_is_reset_7880();
#elif defined(CONFIG_SOC_EXYNOS7570)
	errorcode = fimc_is_reset_7570();
#else
	errorcode = 0xFFFF;
	err_sfr("Err: check project ID: %x\n", errorcode);
#endif

	return errorcode;
}

int fimc_is_sfr_test(void)
{
	int errorcode = 0;

	info_sfr("fimc_is_sfr_test start\n");

	fimc_is_reset();

#if defined(CONFIG_SOC_EXYNOS8895)
	errorcode = fimc_is_sfr_test_8895();
#elif defined(CONFIG_SOC_EXYNOS7880)
	errorcode = fimc_is_sfr_test_7880();
#elif defined(CONFIG_SOC_EXYNOS7570)
	errorcode = fimc_is_sfr_test_7570();
#else
	errorcode = 0xFFFF;
	err_sfr("Err: check project ID: %x\n", errorcode);
#endif

	info_sfr("fimc_is_sfr_test uErrorCode: 0x%x\n", errorcode);

	return errorcode;
}

