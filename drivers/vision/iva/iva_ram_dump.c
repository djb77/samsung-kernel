#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/namei.h>
#include <linux/security.h>

#include "regs/iva_base_addr.h"
#include "iva_ram_dump.h"
#include "iva_ipc_queue.h"
#include "iva_mbox.h"
#include "iva_ipc_header.h"

#define CMSG_REQ_FATAL_HEADER	(0x1)
#define MAX_DUMP_PATH_SIZE	(256)

#define DFT_RAMDUMP_OUT_DIR	"/data/iva_ramdump"		/* under <exec>/ramdump */
#define DFT_RAMDUMP_OUTPUT(f)	(DFT_RAMDUMP_OUT_DIR "/" f)

#define RAMDUMP_DATE_FORMAT	"YYYYMMDD.HHMMSS"
#define RAMDUMP_FILENAME_PREFIX	"iva_ramdump"
#define RAMDUMP_FILENAME_EXT	"bin"

static int iva_ram_dump_create_directory(const char *dir_path, umode_t mode)
{
	struct dentry *dentry;
	struct path path;
	int err = 0;
	/*
	 * Get the parent directory, calculate the hash for last
	 * component.
	 */
	dentry = kern_path_create(0/*AT_FDCWD*/, dir_path, &path,
			LOOKUP_FOLLOW | LOOKUP_DIRECTORY);
	err = PTR_ERR(dentry);
	if (IS_ERR(dentry)) {
		/* already exist */
		if (err == -EEXIST)
			return 0;

		pr_err("%s() Failed to create directory(%s, %d)\n",
				__func__, dir_path, err);
		return err;
	}
	/*
	 * All right, let's create it.
	 */
	err = security_path_mkdir(&path, dentry, mode);
	if (!err) {
		err = vfs_mkdir(path.dentry->d_inode, dentry, mode);
		if (err) {
			pr_err("%s() Failed to create node for directory(%s, %d)\n",
				__func__, dir_path, err);
			return err;
		}
	}
	done_path_create(&path, dentry);
	return err;
}


ssize_t iva_ram_dump_write_file(const char *o_file,
		const char *mem, ssize_t req_size)
{
	struct file *dump_fp;
	ssize_t write_bytes = 0;

	if (!o_file)
		return 0;

	if (!req_size)
		return 0;

	dump_fp = filp_open(o_file, O_CREAT | O_WRONLY,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (IS_ERR_OR_NULL(dump_fp)) {
		pr_err("%s() Failed to open file (%s, %d)\n", __func__,
				o_file, (int)PTR_ERR(dump_fp));
		return 0;
	}

	write_bytes = kernel_write(dump_fp, mem, req_size, 0);

	filp_close(dump_fp, NULL);

	return write_bytes;
}


static void iva_ram_dump_work(struct work_struct *work)
{
	int		ret;
	time_t		now_time = get_seconds();
	struct tm	now_tm;
	char	dump_fn[MAX_DUMP_PATH_SIZE];
	char	timebuf[sizeof(RAMDUMP_DATE_FORMAT) + 1] = RAMDUMP_DATE_FORMAT;
	int	bin_size;
	void __iomem	*mcu_va, *vcf_va;
	struct iva_dev_data	*iva = container_of(work,
					struct iva_dev_data, mcu_rd_work);
	struct device		*dev = iva->dev;
	struct ipc_res_param	res_param;

	time_to_tm(now_time, 0, &now_tm);
	snprintf(timebuf, sizeof(timebuf), "%04ld%02d%02d.%02d%02d%02d",
			now_tm.tm_year + 1900,
			now_tm.tm_mon + 1,
			now_tm.tm_mday,
			now_tm.tm_hour,
			now_tm.tm_min,
			now_tm.tm_sec);

	dev_info(dev, "%s() ramdump work on %s\n", __func__, timebuf);

	ret = iva_ram_dump_create_directory(DFT_RAMDUMP_OUT_DIR,
				S_IRWXU | S_IRWXG | S_IRWXO);
	if (ret) {
		dev_err(dev, "%s() fail to create dir for mcu dump, ret(%d)\n",
			__func__, ret);
		goto snd_msg;
	}

	/* save iva sram to file */
	snprintf(dump_fn, sizeof(dump_fn), "%s.%s.%s",
			DFT_RAMDUMP_OUTPUT(RAMDUMP_FILENAME_PREFIX),
			timebuf,
			RAMDUMP_FILENAME_EXT);
	dev_info(dev, "will create %s\n", dump_fn);

	mcu_va = iva->iva_va + IVA_VMCU_MEM_BASE_ADDR;

	bin_size = iva_ram_dump_write_file(dump_fn,
			(const char *) mcu_va, VMCU_MEM_SIZE);
	dev_info(dev, "%s() created %s of 0x%x size\n", __func__,
			dump_fn, bin_size);

	/* save iva vcf to file */
	snprintf(dump_fn, sizeof(dump_fn), "%s.%s.vcf.%s",
			DFT_RAMDUMP_OUTPUT(RAMDUMP_FILENAME_PREFIX),
			timebuf,
			RAMDUMP_FILENAME_EXT);

	vcf_va = iva->iva_va + IVA_VCF_BASE_ADDR;
	bin_size = iva_ram_dump_write_file(dump_fn,
			(const char *)vcf_va, VCF_MEM_SIZE);

	dev_info(dev, "created %s of 0x%x size\n", dump_fn, bin_size);

snd_msg:
	/* notify the error state to user */
	res_param.header	= IPC_MK_HDR(ipc_func_iva_ctrl, 0x0, 0x0);
	res_param.rep_id	= 0xdd;
	res_param.ret		= (uint32_t) -1;
	res_param.ipc_cmd_type	= 21;	/* SYS_ERR */
	iva_ipcq_send_rsp_k(&iva->mcu_ipcq, &res_param, false);

	return;
}

static bool iva_ram_dump_handler(struct iva_mbox_cb_block *ctrl_blk, uint32_t msg)
{
	struct iva_dev_data *iva = (struct iva_dev_data *) ctrl_blk->priv_data;

	dev_info(iva->dev, "%s() msg(0x%x)\n", __func__, msg);

	queue_work(system_wq, &iva->mcu_rd_work);

	return true;
}

struct iva_mbox_cb_block iva_rd_cb = {
	.callback	= iva_ram_dump_handler,
	.priority	= 0,
};


int32_t iva_ram_dump_init(struct iva_dev_data *iva)
{
	iva_rd_cb.priv_data = iva;
	iva_mbox_callback_register(mbox_msg_fatal, &iva_rd_cb);

	INIT_WORK(&iva->mcu_rd_work, iva_ram_dump_work);

	dev_dbg(iva->dev, "%s() success!!!\n", __func__);
	return 0;
}

int32_t iva_ram_dump_deinit(struct iva_dev_data *iva)
{
	iva_mbox_callback_unregister(&iva_rd_cb);

	iva_rd_cb.priv_data = NULL;

	dev_dbg(iva->dev, "%s() success!!!\n", __func__);
	return 0;
}
