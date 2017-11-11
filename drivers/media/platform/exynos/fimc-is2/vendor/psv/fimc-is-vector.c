#include <linux/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/exynos_ion.h>

#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/zlib.h>

#include <linux/vmalloc.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>

#include "fimc-is-config.h"
#include "fimc-is-binary.h"
#include "fimc-is-mem.h"
#include "fimc-is-vector.h"
#include "fimc-is-vender-specific.h"

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

static ulong make_chksum_64(void *kva, size_t size)
{
	ulong result = 0;
	ulong *data = (ulong *)kva;
	int i;

	for (i = 0; i < (size >> 3); i++)
		result += data[i];

	return result;
}

static struct fimc_is_priv_buf *fimc_is_vector_pbuf_alloc(
	struct fimc_is_vender *vender, size_t size)
{
	struct fimc_is_core *core;
	struct fimc_is_resourcemgr *rscmgr;
	struct fimc_is_vender_specific *priv;
	struct fimc_is_priv_buf *pbuf;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	core = container_of(vender, struct fimc_is_core, vender);
	rscmgr = &core->resourcemgr;

	pbuf = CALL_PTR_MEMOP(&rscmgr->mem, alloc, priv->alloc_ctx, size, 0);
	if (IS_ERR(pbuf)) {
		err_vec("failed to allocate buffer for size: 0x%zx", size);
		return ERR_PTR(-ENOMEM);
	}

	return pbuf;
}

#define HEAD_CRC	2
#define EXTRA_FIELD	4
#define ORIG_NAME	8
#define COMMENT		0x10
#define RESERVED	0xe0
static int strip_gzip_hdr(void *data, size_t size)
{
	char *hdr = data;
	int hdrsize = 10;
	int flags;

	if (size <= hdrsize)
		return -EINVAL;

	/* Check for gzip magic number */
	if ((hdr[0] == 0x1f) && (hdr[1] == 0x8b)) {
		flags = hdr[3];

		if (hdr[2] != Z_DEFLATED || (flags & RESERVED) != 0) {
			err_vec("bad gzipped DMA file");
			return -EINVAL;
		}

		if ((flags & EXTRA_FIELD) != 0)
			hdrsize = 12 + hdr[10] + (hdr[11] << 8);
		if ((flags & ORIG_NAME) != 0)
			while (hdr[hdrsize++] != 0)
				;
		if ((flags & COMMENT) != 0)
			while (hdr[hdrsize++] != 0)
				;
		if ((flags & HEAD_CRC) != 0)
			hdrsize += 2;

		if (hdrsize >= size) {
			err_vec("ran out of data in header");
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	dbg_vec("stripped gzip header size: %d\n", hdrsize);

	return hdrsize;
}

/*
 * [B]Allocate a private buffer for DMA.
 * [B] Load a file into the buffer.
 *    [I] If checksum for input is necessary, make it.
 *    [O] Make a expected checksum with golden output.
 *	  [O] Write 0s to output buffer.
 * [B] Clean cache for the DMA buffer to use it in device space
 * [B] Add the entry to the list.
 */
static int __add_dma_cfg_entry(struct fimc_is_vender *vender,
	struct vector_dma *dma)
{
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	int ret, hdr_size, decomp_size;
	struct fimc_is_binary bin;
	struct fimc_is_binary compressed_bin;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	dma->pbuf = fimc_is_vector_pbuf_alloc(vender, dma->size);
	if (IS_ERR_OR_NULL(dma->pbuf)) {
		err_vec("failed to alloc DMA buffer for %pR size: 0x%08x",
				dma, dma->size);
		return ret;
	}

	bin.data = (void *)CALL_BUFOP(dma->pbuf, kvaddr, dma->pbuf);
	bin.size = dma->size;

	if (cfg->item.compressed) {
		ret = request_binary(&compressed_bin, FIMC_IS_ISP_LIB_SDCARD_PATH,
					dma->filename, NULL);
		if (!ret) {
			hdr_size = strip_gzip_hdr(compressed_bin.data, compressed_bin.size);
			/* minimum header size is 10 bytes */
			if (hdr_size > 0) {
				decomp_size = zlib_inflate_blob(bin.data, bin.size,
							(void *)((char *)compressed_bin.data + hdr_size),
							compressed_bin.size - hdr_size);

				if (decomp_size != dma->size) {
					err_vec("failed to decompressed 0x%08x:0x%08x",
							dma->size, decomp_size);
					ret = -EINVAL;
				}
			}
		}
	} else {
		ret = get_filesystem_binary(dma->filename, &bin);
	}

	if (ret) {
		if (dma->dir == DMA_DIR_INPUT) {
			err_vec("failed to copy file to DMA[I] buffer for %pR\n"
					"\t\t\t\tkvaddr: %p size: 0x%zx filename: %s",
					dma, bin.data, bin.size, dma->filename);

			goto err_get_file;
		} else if (dma->dir == DMA_DIR_OUTPUT) {
			if (dma->chksum_expect == 0)
			/*
			 * If we are here, it failed to load DMA output file,
			 * and we already failed to get a expected
			 * checksum value from the DMA configuration file.
			 * So, we cannot use the checksum verifiaction.
			 */
			err_vec("failed to get a expected checksum value\n"
					"\t\t\t\tofs: 0x%08x, size: 0x%zx filename: %s",
					dma->sfr_ofs, bin.size, dma->filename);
		}
	} else {
		dma->chksum_expect = 0;
	}

#ifdef INPUT_CHKSUM
	if (dma->dir == DMA_DIR_INPUT) {
		dma->chksum_input = make_chksum_64(bin.data, bin.size);
	} else if (dma->dir == DMA_DIR_OUTPUT) {
#else
	if (dma->dir == DMA_DIR_OUTPUT) {
#endif
		/* A DMA output file was loaded successfully */
		if (dma->chksum_expect == 0)
			dma->chksum_expect = make_chksum_64(bin.data, bin.size);
		memset(bin.data, 0x00, dma->size);
	}

	CALL_VOID_BUFOP(dma->pbuf, sync_for_device, dma->pbuf, 0,
				dma->size, DMA_TO_DEVICE);

	list_add_tail(&dma->list, &cfg->dma);

	return 0;

err_get_file:
	CALL_VOID_BUFOP(dma->pbuf, free, dma->pbuf);

	return ret;
}

/*
 * Remove all DMA entries from the head.
 * Free the DMA private buffer.
 * Free a DMA entry itself.
 */
static void __flush_dma_cfg_entries(struct vector_cfg *cfg)
{
	struct vector_dma *dma;

	while (!list_empty(&cfg->dma)) {
		dma = list_entry((&cfg->dma)->next, struct vector_dma, list);
		dbg_vec("flushing DMA[%c] ofs: 0x%08x, size: 0x%08x\n",
				dma->dir ? 'I' : 'O', dma->sfr_ofs, dma->size);
		CALL_VOID_BUFOP(dma->pbuf, free, dma->pbuf);
		list_del(&dma->list);
		kfree(dma);
	}
}

#define is_valid_line(cols, entry)		\
	((cols == NUM_OF_COL_DMA_CFG) &&	\
	 ((entry->dir == DMA_DIR_INPUT) ||	\
	  (entry->dir == DMA_DIR_OUTPUT) ||	\
	  (entry->dir == DMA_DIR_IGNORE)))
static int fimc_is_vector_dma_load(struct fimc_is_vender *vender, int id)
{
	int ret;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	char *dma_cfg_filename;
	struct fimc_is_binary dma_cfg_bin;
	char *dma_cfg_data;
	size_t cur_pos = 0;
	int cols;
	unsigned int ofs;
	struct vector_dma *entry = NULL;
	char filename[SIZE_OF_NAME];

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	dma_cfg_filename = __getname();
	if (unlikely(!dma_cfg_filename))
		return -ENOMEM;

	snprintf(dma_cfg_filename, PATH_MAX, "%s%d/%s_%d", "vector",
			id, "dma", cfg->framecnt);
	ret = request_binary(&dma_cfg_bin, FIMC_IS_ISP_LIB_SDCARD_PATH,
				dma_cfg_filename, NULL);
	if (ret) {
		err_vec("failed to load vector DMA configuration (%d): name: %s",
				ret, dma_cfg_filename);
		goto err_req_dma_cfg_bin;
	} else {
		info_vec("vector DMA info - size: 0x%zx name: %s\n",
				dma_cfg_bin.size, dma_cfg_filename);
	}

	dma_cfg_data = (char *)dma_cfg_bin.data;
	while (cur_pos < dma_cfg_bin.size) {
		if (!entry) {
			entry = kzalloc(sizeof(struct vector_dma), GFP_KERNEL);
			if (!entry) {
				err_vec("failed to allocate DMA entry");
				__flush_dma_cfg_entries(cfg);
				err_vec("flushed all DMA entries");
				ret = -EINVAL;
				goto err_alloc_dma_entry;
			}
		}

		cols = sscanf(dma_cfg_data, "%x %x %d %127s\n", &entry->sfr_ofs,
					&entry->size, &entry->dir, filename);

		if (is_valid_line(cols, entry)) {
			dbg_vec("DMA[%c] ofs: 0x%08x, size: 0x%08x, filename: %s\n",
					entry->dir ? 'I' : 'O', entry->sfr_ofs,
					entry->size, filename);

					/* in case of using a given checksum value */
					if (entry->dir == DMA_DIR_OUTPUT)
						sscanf(filename, "%lx", &entry->chksum_expect);

					if (cfg->item.compressed)
						/* added only a patial path to use request_binary */
						snprintf(entry->filename, SIZE_OF_NAME, "%s%d/%s",
								"vector", id, filename);
					else
						/* added a full path to entry's filename */
						snprintf(entry->filename, SIZE_OF_NAME, "%s%s%d/%s",
								FIMC_IS_ISP_LIB_SDCARD_PATH, "vector", id,
								filename);

			if (__add_dma_cfg_entry(vender, entry)) {
				err_vec("failed to add DMA entry");
				__flush_dma_cfg_entries(cfg);
				err_vec("flushed all DMA entries");
				ret = -EINVAL;
				goto err_add_dma_entry;
			}
			entry = NULL;
		}

		ofs = get_newline_offset(dma_cfg_data, dma_cfg_bin.size - cur_pos);
		if (ofs) {
			cur_pos += ofs;
			dma_cfg_data += ofs;
		} else {
			break;
		}
	}

err_add_dma_entry:
	if (entry)
		kfree(entry);

err_alloc_dma_entry:
	release_binary(&dma_cfg_bin);

err_req_dma_cfg_bin:
	__putname(dma_cfg_filename);

	return ret;
}

static void fimc_is_vector_dma_set(struct fimc_is_vender *vender, int id)
{
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	struct vector_dma *dma;
	dma_addr_t dva;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	info_vec("start DMA address setting\n");

	list_for_each_entry(dma, &cfg->dma, list) {
		dva = CALL_BUFOP(dma->pbuf, dvaddr, dma->pbuf);
		if (cfg->item.no_fimcis)
			__raw_writel((u32)dva, cfg->baseaddr + dma->sfr_ofs);
		else {
			if ((dma->sfr_ofs == 0x13141004) || (dma->sfr_ofs == 0x13149004))
				dva += 0x18;

			if ((dma->sfr_ofs == 0x13141204) || (dma->sfr_ofs == 0x13149204))
				dva += 0x8;

			vector_writel((u32)dva, cfg->baseaddr, dma->sfr_ofs);
		}

		dbg_vec("DMA ofs: 0x%08x, size: 0x%08x, dva: %pa\n",
				dma->sfr_ofs, dma->size, &dva);
	}
}

static int fimc_is_vector_dma_dump(struct fimc_is_vender *vender, int id)
{
	int ret = 0;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	struct vector_dma *dma;
	char *filename;
	struct fimc_is_binary bin;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	filename = __getname();

	if (unlikely(!filename))
		return -ENOMEM;

	list_for_each_entry(dma, &cfg->dma, list) {
		/* 0: none, 1: output, 2: input */
		if (cfg->item.dump_dma & (1 << dma->dir)) {
			snprintf(filename, PATH_MAX, "%s%s%d/dump/%08x_%08x_%d.raw",
					FIMC_IS_ISP_LIB_SDCARD_PATH,"vector",
					id, dma->sfr_ofs, dma->size, cfg->framecnt);

			dbg_vec("dumping DMA[%c] ofs: 0x%08x, size: 0x%08x to %s\n",
					dma->dir ? 'I' : 'O', dma->sfr_ofs, dma->size, filename);

			bin.data = (void *)CALL_BUFOP(dma->pbuf, kvaddr, dma->pbuf);
			bin.size = dma->size;

			ret = put_filesystem_binary(filename, &bin, O_SYNC | O_TRUNC | O_CREAT | O_WRONLY);
			if (ret) {
				err_vec("failed to dump DMA to %s (%d)", filename, ret);
				break;
			}
		}
	}

	__putname(filename);

	return ret;
}

/*
 * Add the entry to the list.
 */
static int __add_crc_cfg_entry(struct fimc_is_vender *vender,
	struct vector_crc *crc)
{
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	list_add_tail(&crc->list, &cfg->crc);

   return 0;
}

/*
 * Remove all DMA entries from the head.
 * Free a DMA entry itself.
 */
static void __flush_crc_cfg_entries(struct vector_cfg *cfg)
{
	struct vector_crc *crc;

	while (!list_empty(&cfg->crc)) {
		crc = list_entry((&cfg->crc)->next, struct vector_crc, list);
		dbg_vec("flusing CRC addr: 0x%08x, value: 0x%08x, mask: 0x%08x\n",
				crc->sfr_addr, crc->value, crc->sfr_mask);
		list_del(&crc->list);
		kfree(crc);
	}
}

static int fimc_is_vector_crc_load(struct fimc_is_vender *vender, int id)
{
	int ret;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	char *crc_cfg_filename;
	struct fimc_is_binary crc_cfg_bin;
	char *crc_cfg_data;
	size_t cur_pos = 0;
	int cols;
	unsigned int ofs;
	struct vector_crc *entry = NULL;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	crc_cfg_filename = __getname();
	if (unlikely(!crc_cfg_filename))
		return -ENOMEM;

	snprintf(crc_cfg_filename, PATH_MAX, "%s%d/%s_%d", "vector",
			id, "crc", cfg->framecnt);
	ret = request_binary(&crc_cfg_bin, FIMC_IS_ISP_LIB_SDCARD_PATH,
				crc_cfg_filename, NULL);
	if (ret) {
		err_vec("failed to load vector CRC configuration (%d): name: %s",
				ret, crc_cfg_filename);
		goto err_req_crc_cfg_bin;
	} else {
		info_vec("vector CRC info - size: 0x%zx name: %s\n",
				crc_cfg_bin.size, crc_cfg_filename);
	}

	crc_cfg_data = (char *)crc_cfg_bin.data;
	while (cur_pos < crc_cfg_bin.size) {
		if (!entry) {
			entry = kzalloc(sizeof(struct vector_crc), GFP_KERNEL);
			if (!entry) {
				err_vec("failed to allocate CRC entry");
				__flush_crc_cfg_entries(cfg);
				err_vec("flushed all CRC entries");
				ret = -EINVAL;
				goto err_alloc_crc_entry;
			}
		}

		cols = sscanf(crc_cfg_data, "%x %x %x\n", &entry->sfr_addr,
					&entry->value, &entry->sfr_mask);

		if (cols == NUM_OF_COL_CRC_CFG) {
			dbg_vec("CRC addr: 0x%08x, value: 0x%08x, mask: 0x%08x\n",
					entry->sfr_addr, entry->value, entry->sfr_mask);

			if (__add_crc_cfg_entry(vender, entry)) {
				err_vec("failed to add CRC entry");
				__flush_crc_cfg_entries(cfg);
				err_vec("flushed all CRC entries");
				ret = -EINVAL;
				goto err_add_crc_entry;
			}
			entry = NULL;
		}

		ofs = get_newline_offset(crc_cfg_data, crc_cfg_bin.size - cur_pos);
		if (ofs) {
			cur_pos += ofs;
			crc_cfg_data += ofs;
		} else {
			break;
		}
	}

err_add_crc_entry:
	if (entry)
		kfree(entry);

err_alloc_crc_entry:
	release_binary(&crc_cfg_bin);

err_req_crc_cfg_bin:
	__putname(crc_cfg_filename);

	return ret;
}

static int fimc_is_vector_cfg_load(struct fimc_is_vender *vender, int id)
{
	int ret;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *config;
	char *config_filename;
	struct fimc_is_binary config_bin;
	char *config_data;
	size_t cur_pos = 0;
	int cols;
	unsigned int ofs;
	int i;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	config = &priv->vector_cfg;

	config_filename = __getname();
	if (unlikely(!config_filename))
		return -ENOMEM;

	snprintf(config_filename, PATH_MAX, "%s%d/%s", "vector", id, "config");
	ret = request_binary(&config_bin, FIMC_IS_ISP_LIB_SDCARD_PATH,
				config_filename, NULL);
	if (ret) {
		err_vec("failed to load vector configuration (%d): name: %s",
				ret, config_filename);
		goto err_req_config_bin;
	} else {
		info_vec("vector configuration info - size: 0x%zx name: %s\n",
				config_bin.size, config_filename);
	}

	config_data = (char *)config_bin.data;

	/* name */
	if (cur_pos >= config_bin.size) {
		err_vec("could not start loading configuration");
		ret = -EINVAL;
		goto err_get_config_item;
	}
	cols = sscanf(config_data, "%127s\n", config->name);
	if (cols != NUM_OF_COL_CONFIG) {
		err_vec("could not load configuration - %s", "name");
		ret = -EINVAL;
		goto err_get_config_item;
	}
	dbg_vec("Configuration name: %s\n", config->name);

	/* numeric items */
	for (i = 0; i < NUM_OF_NUMERIC_CFGS; i++) {
		ofs = get_newline_offset(config_data, config_bin.size - cur_pos);
		if (ofs) {
			cur_pos += ofs;
			config_data += ofs;
			if (cur_pos >= config_bin.size) {
				err_vec("could not go further more after - %s", cfg_name[i]);
				ret = -EINVAL;
				goto err_get_config_item;
			}

			cols = sscanf(config_data, "%d\n", &config->items[i]);
			if (cols != NUM_OF_COL_CONFIG) {
				err_vec("could not load configuration - %s",
						cfg_name[i + OFS_OF_NUMERIC_CFGS]);
				ret = -EINVAL;
				goto err_get_config_item;
			}

			dbg_vec("Configuration %s: %d\n",
					cfg_name[i + OFS_OF_NUMERIC_CFGS], config->items[i]);
		} else {
			err_vec("could not go further more after - %s", cfg_name[i]);
			ret = -EINVAL;
			goto err_get_config_item;
		}
	}

#ifdef USE_ION_DIRECTLY
	config->client = exynos_ion_client_create("fimc_is_vector");
	if (IS_ERR(client)) {
		err_vec("failed to create ion client for vector\n");
		ret = -EINVAL;
		goto err_create_ion_client;
	}
#endif

	if (config->item.no_fimcis)
		config->baseaddr = ioremap_nocache(config->item.no_fimcis_baseaddr, IO_MEM_SIZE);
	else
		config->baseaddr = ioremap_nocache(IO_MEM_BASE, IO_MEM_SIZE);

	if (!config->baseaddr) {
		err_vec("failed to map IO memory base");
		ret = -EINVAL;
		goto err_remap_iomem;
	}

	config->framecnt = 0;
	INIT_LIST_HEAD(&config->dma);
	INIT_LIST_HEAD(&config->crc);
	atomic_set(&config->done_3aa0, 0);
	atomic_set(&config->done_3aa1, 0);
	atomic_set(&config->done_isp0, 0);
	atomic_set(&config->done_isp1, 0);
	atomic_set(&config->done_mcsc0, 0);
	atomic_set(&config->done_mcsc1, 0);
	atomic_set(&config->done_vra0, 0);
	atomic_set(&config->done_vra1, 0);
	atomic_set(&config->done_dcp, 0);
	atomic_set(&config->done_srdz, 0);

	config->async_id = -1;

err_remap_iomem:
#ifdef USE_ION_DIRECTLY
err_create_ion_client:
#endif
err_get_config_item:
	release_binary(&config_bin);

err_req_config_bin:
	__putname(config_filename);

	return ret;
}

static void fimc_is_vector_cfg_unload(struct fimc_is_vender *vender, int id)
{
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *config;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	config = &priv->vector_cfg;

	if (config->baseaddr)
		iounmap(config->baseaddr);
}

static int fimc_is_vector_load_n_set(struct fimc_is_vender *vender,
	int id, const char* type)
{
	int ret;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	char *filename;
	struct fimc_is_binary bin;
	char *data;
	size_t cur_pos = 0;
	int cols;
	unsigned int ofs;
	unsigned int sfr_ofs, sfr_val;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	filename = __getname();
	if (unlikely(!filename))
		return -ENOMEM;

	snprintf(filename, PATH_MAX, "%s%d/%s_%d", "vector",
			id, type, cfg->framecnt);
	ret = request_binary(&bin, FIMC_IS_ISP_LIB_SDCARD_PATH, filename, NULL);
	if (ret) {
		err_vec("failed to load vector %s (%d): name: %s", type, ret,
				filename);
		goto err_req_bin;
	} else {
		info_vec("vector %s info - size: 0x%zx name: %s\n",
				type, bin.size, filename);
	}

	data = (char *)bin.data;
	while (cur_pos < bin.size) {
		cols = sscanf(data, "%x %x\n", &sfr_ofs, &sfr_val);
		if (cols == NUM_OF_COL_SFR_CFG) {
			/* to skip too many meaningless log */
			if (strcmp(type, "sfr"))
				dbg_vec("ofs: 0x%08x, val: 0x%08x\n", sfr_ofs, sfr_val);
			/*
			 * WARN: we are assuming that each SFR set represents as
			 *		 17 characters at least like below
			 *		 'XXXXXXXX XXXXXXXX';
			 */
			cur_pos += MIN_LEN_OF_SFR_CFG;
			data += MIN_LEN_OF_SFR_CFG;

			if (cfg->item.no_fimcis) {
				__raw_writel(sfr_val, cfg->baseaddr + sfr_ofs);
			} else {
				if ((sfr_ofs >= IO_MEM_BASE) || sfr_ofs < (IO_MEM_BASE + IO_MEM_SIZE))
					vector_writel(sfr_val, cfg->baseaddr, sfr_ofs);
				else
					err_vec("invalid SFR address: base: %p, addr: 0x%016x\n",
							cfg->baseaddr, sfr_ofs);
			}
		}

		ofs = get_newline_offset(data, bin.size - cur_pos);
		if (ofs) {
			cur_pos += ofs;
			data += ofs;
		} else {
			break;
		}
	}

	/* TODO: remove it */
#if defined(CONFIG_SOC_EXYNOS7880)
	if (!strcmp(type, "sfr")) {
		__raw_writel(0x1, cfg->baseaddr + 0x3126c);
		__raw_writel(0x1, cfg->baseaddr + 0x3136c);
		__raw_writel(0x1, cfg->baseaddr + 0x3146c);
		__raw_writel(0x1, cfg->baseaddr + 0x3156c);
	}
#endif
	release_binary(&bin);

err_req_bin:
	__putname(filename);

	return ret;
}

static int fimc_is_vector_sysmmu_resume(struct fimc_is_vender *vender)
{
	struct fimc_is_vender_specific *priv;
#ifdef USE_IOMMU
	struct fimc_is_core *core;
	struct fimc_is_resourcemgr *rscmgr;
#else
	struct vector_cfg *cfg;
#endif

	priv = (struct fimc_is_vender_specific *)vender->private_data;

#ifdef USE_IOMMU
	core = container_of(vender, struct fimc_is_core, vender);
	rscmgr = &core->resourcemgr;

	return CALL_MEMOP(&rscmgr->mem, resume, priv->alloc_ctx);
#else
	cfg = &priv->vector_cfg;

#ifndef CONFIG_SOC_EXYNOS8895
	__raw_writel(0x00000000, cfg->baseaddr + SYSMMU_ISP0_OFS);
#ifndef CONFIG_SOC_EXYNOS7570
	__raw_writel(0x00000000, cfg->baseaddr + SYSMMU_ISP1_OFS);
	__raw_writel(0x00000000, cfg->baseaddr + SYSMMU_ISP2_OFS);
#endif
#endif
#endif

	return 0;
}

static void fimc_is_vector_sysmmu_suspend(struct fimc_is_vender *vender)
{
#ifdef USE_IOMMU
	struct fimc_is_vender_specific *priv;
	struct fimc_is_core *core;
	struct fimc_is_resourcemgr *rscmgr;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	core = container_of(vender, struct fimc_is_core, vender);
	rscmgr = &core->resourcemgr;

	CALL_VOID_MEMOP(&rscmgr->mem, suspend, priv->alloc_ctx);
#endif
}

#if 0
static irqreturn_t vector_isp0_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int intstatus, save_val;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

#if defined(CONFIG_SOC_EXYNOS7880)
	/* backup interrupt masking */
	save_val = __raw_readl(cfg->baseaddr + 0x02508);
	__raw_writel(0x0, cfg->baseaddr + 0x02508);

	intstatus = __raw_readl(cfg->baseaddr + 0x02504);

	/* interrupt clear */
	__raw_writel(intstatus, cfg->baseaddr + 0x02510);

	/* frame start -> disable for one shot */
	if (((intstatus >> 1) & 0x1) == 0x1) {
		atomic_set(&cfg->done_isp0, 1);
		wake_up(&cfg->wait);
	}

	/* restore interrupt masking */
	__raw_writel(save_val, cfg->baseaddr + 0x02508);
#elif defined(CONFIG_SOC_EXYNOS7570)
	/* ISP v3.xxx */
	/* backup interrupt masking */
	save_val = __raw_readl(cfg->baseaddr + 0x03608);
	__raw_writel(0x0, cfg->baseaddr + 0x03608);

	intstatus = __raw_readl(cfg->baseaddr + 0x03604);

	/* interrupt clear */
	__raw_writel(intstatus, cfg->baseaddr + 0x03604);

	/* frame start -> disable for one shot */
	if (((intstatus >> 6) & 0x1) == 0x1) {
		__raw_writel(0x0, cfg->baseaddr + 0x03000);
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_ISP0].start_time = cpu_clock(UINT_MAX);
	}
	/* frame end */
	if (((intstatus >> 2) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_ISP0].end_time = cpu_clock(UINT_MAX);
		atomic_set(&cfg->done_isp0, 1);
		wake_up(&cfg->wait);
	}

	/* restore interrupt masking */
	__raw_writel(save_val, cfg->baseaddr + 0x03608);
#elif defined(CONFIG_SOC_EXYNOS8895)
	/*TODO: Kangchen ISP interrupt register offset check */
	/* backup interrupt masking */
	save_val = vector_readl(cfg->baseaddr, 0x13043518);
	vector_writel(0x0, cfg->baseaddr, 0x13043518);

	intstatus = vector_readl(cfg->baseaddr, 0x13043514);

	/* interrupt clear */
	vector_writel(intstatus, cfg->baseaddr, 0x13043520);

	/* frame start -> disable for one shot */
	if (((intstatus >> 1) & 0x1) == 0x1) {
		atomic_set(&cfg->done_isp0, 1);
		wake_up(&cfg->wait);
	}

	/* restore interrupt masking */
	vector_writel(save_val, cfg->baseaddr, 0x13043518);
#endif
	return IRQ_HANDLED;
}

#ifndef CONFIG_SOC_EXYNOS7570
static irqreturn_t vector_3aa0_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int intstatus, save_val;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

#if defined(CONFIG_SOC_EXYNOS8895)
	/* interrupt masking */
		save_val = vector_readl(cfg->baseaddr, 0x13130108);
		vector_writel(0x0, cfg->baseaddr, 0x131380108);

		intstatus = vector_readl(cfg->baseaddr, 0x131380104);

	/* interrupt clear */
		vector_writel(intstatus, cfg->baseaddr, 0x131380110);

		/* frame start -> disable for one shot */
		if (((intstatus >> 1) & 0x1) == 0x1) {
			atomic_set(&cfg->done_3aa0, 1);
			wake_up(&cfg->wait);

		}

		vector_writel(save_val, cfg->baseaddr, 0x131380108);
#else
	/* interrupt masking */
		save_val = __raw_readl(cfg->baseaddr + 0x80108);
		__raw_writel(0x0, cfg->baseaddr + 0x80108);

		intstatus = __raw_readl(cfg->baseaddr + 0x80104);

	/* interrupt clear */
		__raw_writel(intstatus, cfg->baseaddr + 0x80110);

		/* frame start -> disable for one shot */
		if (((intstatus >> 1) & 0x1) == 0x1) {
			atomic_set(&cfg->done_3aa0, 1);
	wake_up(&cfg->wait);

		}

		__raw_writel(save_val, cfg->baseaddr + 0x80108);
#endif

	return IRQ_HANDLED;
}
#endif
#endif

#ifdef CONFIG_SOC_EXYNOS8895
#if 0
static irqreturn_t vector_3aaw_0_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int intstatus;
	unsigned int save_val;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	save_val = vector_readl(cfg->baseaddr, 0x13030108);
	vector_writel(0x0, cfg->baseaddr, 0x13030108);

	intstatus = vector_readl(cfg->baseaddr, 0x1303010c);

	/* interrupt clear */
	vector_writel(intstatus, cfg->baseaddr, 0x13030110);

	/* frame start -> disable for one shot */
	if (((intstatus >> 1) & 0x1) == 0x1) {
		vector_writel(0x0, cfg->baseaddr, 0x13030000);
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_3AAW_0].start_time = cpu_clock(UINT_MAX);
	}

	if (((intstatus >> 0) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_3AAW_0].end_time = cpu_clock(UINT_MAX);
		atomic_set(&cfg->mcscdone, 1);
		wake_up(&cfg->wait);
	}

	vector_writel(save_val, cfg->baseaddr, 0x13030108);

	return IRQ_HANDLED;
}
#endif

#if 0
static irqreturn_t vector_3aaw_1_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	//unsigned int intstatus;
	//unsigned int save_val;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

#if 0
	save_val = vector_readl(cfg->baseaddr, 0x12C507a4);
	vector_writel(0x0, cfg->baseaddr, 0x12C507a4);

	intstatus = vector_readl(cfg->baseaddr, 0x12C507ac);

	/* interrupt clear */
	vector_writel(intstatus, cfg->baseaddr, 0x12C507ac);

	/* frame start -> disable for one shot */
	if (((intstatus >> 1) & 0x1) == 0x1) {
		vector_writel(0x0, cfg->baseaddr, 0x12C530000);
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_MCSC].start_time = cpu_clock(UINT_MAX);
	}

	if (((intstatus >> 0) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_MCSC].end_time = cpu_clock(UINT_MAX);
		atomic_set(&cfg->mcscdone, 1);
		wake_up(&cfg->wait);
	}

	vector_writel(save_val, cfg->baseaddr, 0x12C507a4);
#endif

	return IRQ_HANDLED;
}
#endif

#if 0

static irqreturn_t vector_mcsc_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int intstatus;
	unsigned int save_val;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	save_val = vector_readl(cfg->baseaddr, 0x12C507a4);
	vector_writel(0x0, cfg->baseaddr, 0x12C507a4);

	intstatus = vector_readl(cfg->baseaddr, 0x12C507ac);

	/* interrupt clear */
	vector_writel(intstatus, cfg->baseaddr, 0x12C507ac);

	/* frame start -> disable for one shot */
	if (((intstatus >> 1) & 0x1) == 0x1) {
		vector_writel(0x0, cfg->baseaddr, 0x12C530000);
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_MCSC].start_time = cpu_clock(UINT_MAX);
	}

	if (((intstatus >> 0) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_MCSC].end_time = cpu_clock(UINT_MAX);
		atomic_set(&cfg->mcscdone, 1);
		wake_up(&cfg->wait);
	}

	vector_writel(save_val, cfg->baseaddr, 0x12C507a4);

	return IRQ_HANDLED;
}
#endif

#elif defined CONFIG_SOC_EXYNOS7570
static irqreturn_t vector_mcsc_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int intstatus;
	unsigned int save_val;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	save_val = __raw_readl(cfg->baseaddr + 0x307a4);
	__raw_writel(0x0, cfg->baseaddr + 0x307a4);

	intstatus = __raw_readl(cfg->baseaddr + 0x307ac);

	/* interrupt clear */
	__raw_writel(intstatus, cfg->baseaddr + 0x307ac);

	/* frame start -> disable for one shot */
	if (((intstatus >> 1) & 0x1) == 0x1) {
		__raw_writel(0x0, cfg->baseaddr + 0x30000);
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_MCSC].start_time = cpu_clock(UINT_MAX);
	}

	if (((intstatus >> 0) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_MCSC].end_time = cpu_clock(UINT_MAX);
		atomic_set(&cfg->mcscdone, 1);
		wake_up(&cfg->wait);
	}

	__raw_writel(save_val, cfg->baseaddr + 0x307a4);

	return IRQ_HANDLED;
}

static irqreturn_t vector_vra1_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int intstatus;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	intstatus = __raw_readl(cfg->baseaddr + 0x4b40c);

	/* interrupt clear */
	__raw_writel(intstatus, cfg->baseaddr + 0x4b410);

	if (((intstatus >> 3) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_VRA1].start_time = cpu_clock(UINT_MAX);
	}

	if (((intstatus >> 2) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_VRA1].end_time = cpu_clock(UINT_MAX);
		atomic_set(&cfg->vra1done, 1);
		wake_up(&cfg->wait);
	}

	return IRQ_HANDLED;
}

static irqreturn_t vector_mfcmcsc_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int intstatus;
	unsigned int save_val;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	save_val = __raw_readl(cfg->baseaddr + 0x07a4);
	__raw_writel(0x0, cfg->baseaddr + 0x07a4);

	intstatus = __raw_readl(cfg->baseaddr + 0x07ac);

	/* interrupt clear */
	__raw_writel(intstatus, cfg->baseaddr + 0x07ac);

	/* frame start -> disable for one shot */
	if (((intstatus >> 1) & 0x1) == 0x1) {
		cfg->time[PDEV_IRQ_NUM_MFCMCSC].start_time = cpu_clock(UINT_MAX);
		__raw_writel(0x0, cfg->baseaddr + 0x0000);
	}

	if (((intstatus >> 0) & 0x1) == 0x1) {
		cfg->time[PDEV_IRQ_NUM_MFCMCSC].end_time = cpu_clock(UINT_MAX);
		atomic_set(&cfg->done_mcsc1, 1);
		wake_up(&cfg->wait);
	}

	__raw_writel(save_val, cfg->baseaddr + 0x07a4);

	return IRQ_HANDLED;
}
#endif

static irqreturn_t vector_3aa0_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int status;
	//static int count = 0;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	status = vector_readl(cfg->baseaddr, 0x1303010c);

	/* interrupt clear */
	vector_writel(status, cfg->baseaddr, 0x13030110);

	/* frame start -> disable for one shot */
	if ((status & 0x1) == 0x1) {
		//count++;
		//if (!(count % 10))
			vector_writel(0x0, cfg->baseaddr, 0x13030000);
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_ISPHQ_0].start_time = local_clock();
	}

	if (((status >> 1) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_3AAW_0].end_time = local_clock();
		atomic_set(&cfg->done_3aa0, 1);
		wake_up(&cfg->wait);
	}

	return IRQ_HANDLED;
}

static irqreturn_t vector_mcsc0_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int mask, status;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	mask = __raw_readl(cfg->baseaddr + 0xc507a4);
	__raw_writel(0x0, cfg->baseaddr + 0xc507a4);

	status = __raw_readl(cfg->baseaddr + 0xc507ac);

	/* interrupt clear */
	__raw_writel(status, cfg->baseaddr + 0xc507ac);

	/* frame start -> disable for one shot */
	if (((status >> 1) & 0x1) == 0x1) {
		__raw_writel(0x0, cfg->baseaddr + 0xc50000);
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_MCSC_0].start_time = local_clock();
	}

	if (((status >> 0) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_MCSC_0].end_time = local_clock();
		atomic_set(&cfg->done_mcsc0, 1);
		wake_up(&cfg->wait);
	}

	__raw_writel(mask, cfg->baseaddr + 0xc507a4);

	return IRQ_HANDLED;
}

static irqreturn_t vector_mcsc1_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int mask, status;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	mask = __raw_readl(cfg->baseaddr + 0xc507a8);
	__raw_writel(0x0, cfg->baseaddr + 0xc507a8);

	status = __raw_readl(cfg->baseaddr + 0xc507b0);

	/* interrupt clear */
	__raw_writel(status, cfg->baseaddr + 0xc507b0);

	/* frame start -> disable for one shot */
	if (((status >> 1) & 0x1) == 0x1) {
		__raw_writel(0x0, cfg->baseaddr + 0xc50004);
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_MCSC_1].start_time = local_clock();
	}

	if (((status >> 0) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_MCSC_1].end_time = local_clock();
		atomic_set(&cfg->done_mcsc1, 1);
		wake_up(&cfg->wait);
	}

	__raw_writel(mask, cfg->baseaddr + 0xc507a8);

	return IRQ_HANDLED;
}

static irqreturn_t vector_isp1_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int status;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	status = vector_readl(cfg->baseaddr, 0x1314350c);

	/* interrupt clear */
	vector_writel(status, cfg->baseaddr, 0x13143510);

	/* frame start -> disable for one shot */
	if ((status & 0x1) == 0x1) {
		vector_writel(0x0, cfg->baseaddr, 0x13140000);
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_ISPHQ_0].start_time = local_clock();
	}

	if (((status >> 1) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_ISPHQ_0].end_time = local_clock();
		atomic_set(&cfg->done_isp1, 1);
		wake_up(&cfg->wait);
	}

	return IRQ_HANDLED;
}

#if 0
static irqreturn_t vector_isp1_isr(int irq, void *data)
{
	struct fimc_is_vender *vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	unsigned int mask, status;

	vender = (struct fimc_is_vender *)data;
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	mask = __raw_readl(cfg->baseaddr + 0x1142608);
	__raw_writel(0x0, cfg->baseaddr + 0x1142608);

	status = __raw_readl(cfg->baseaddr + 0x114260c);

	/* interrupt clear */
	__raw_writel(status, cfg->baseaddr + 0x1142610);

	/* frame start -> disable for one shot */
	if (((status) & 0x1) == 0x1) {
		__raw_writel(0x0, cfg->baseaddr + 0x1140000);
		__raw_writel(0x0, cfg->baseaddr + 0x1140004);
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_ISPHQ_0].start_time = local_clock();
	}

	if (((status >> 1) & 0x1) == 0x1) {
		if (cfg->measure_time_enable)
			cfg->time[PDEV_IRQ_NUM_ISPHQ_0].end_time = local_clock();
		atomic_set(&cfg->done_isp1, 1);
		wake_up(&cfg->wait);
	}

	__raw_writel(mask, cfg->baseaddr + 0x1142608);

	return IRQ_HANDLED;
}
#endif

static int fimc_is_vector_request_irqs(struct fimc_is_vender *vender)
{
	int ret;
	struct fimc_is_vender_specific *priv;
	struct fimc_is_core *core;
	struct vector_cfg *cfg;
	char irq_name[SIZE_OF_NAME];

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	core = container_of(vender, struct fimc_is_core, vender);
	cfg = &priv->vector_cfg;

       /* 3AAW */
	/*
	snprintf(irq_name, SIZE_OF_NAME, "fimc-psv-%d", PDEV_IRQ_NUM_3AAW_0);
	ret = request_irq(cfg->irq_3aaw_0, vector_3aaw_0_isr, 0, irq_name, vender);
	if (ret) {
		err_vec("failed to register isr for %d", PDEV_IRQ_NUM_3AAW_0);
		return ret;
	}

	snprintf(irq_name, SIZE_OF_NAME, "fimc-psv-%d", PDEV_IRQ_NUM_3AAW_1);
	ret = request_irq(cfg->irq_3aaw_1, vector_3aaw_1_isr, 0, irq_name, vender);
	if (ret) {
		err_vec("failed to register isr for %d", PDEV_IRQ_NUM_3AAW_1);
		return ret;
	}
	*/
#if 0
	/* isp */
	snprintf(irq_name, SIZE_OF_NAME, "fimc-psv-%d", PDEV_IRQ_NUM_ISP0);
	ret = request_irq(cfg->irq_isp0, vector_isp0_isr, 0, irq_name, vender);
	if (ret) {
		err_vec("failed to register isr for %d", PDEV_IRQ_NUM_ISP0);
		return ret;
	}

#ifndef CONFIG_SOC_EXYNOS7570
	/* 3aa */
	snprintf(irq_name, SIZE_OF_NAME, "fimc-psv-%d", PDEV_IRQ_NUM_3AA0);
	ret = request_irq(cfg->irq_3aa0, vector_3aa0_isr, 0, irq_name, vender);
	if (ret) {
		err_vec("failed to register isr for %d", PDEV_IRQ_NUM_3AA0);
		return ret;
	}
#endif

#if defined (CONFIG_SOC_EXYNOS7570) || defined (CONFIG_SOC_EXYNOS8895)
	/* mcsc */
	snprintf(irq_name, SIZE_OF_NAME, "fimc-psv-%d", PDEV_IRQ_NUM_MCSC);
	ret = request_irq(cfg->irq_mcsc, vector_mcsc_isr, 0, irq_name, vender);
	if (ret) {
		err_vec("failed to register isr for %d", PDEV_IRQ_NUM_MCSC);
		return ret;
	}
#endif

#ifdef CONFIG_SOC_EXYNOS7570
	/* vra1 */
	snprintf(irq_name, SIZE_OF_NAME, "fimc-psv-%d", PDEV_IRQ_NUM_VRA1);
	ret = request_irq(cfg->irq_vra1, vector_vra1_isr, 0, irq_name, vender);
	if (ret) {
		err_vec("failed to register isr for %d", PDEV_IRQ_NUM_VRA1);
		return ret;
	}
	if (cfg->item.no_fimcis) {
		/* mfc_mcsc */
		snprintf(irq_name, SIZE_OF_NAME, "fimc-psv-%d", PDEV_IRQ_NUM_MFCMCSC);
		ret = request_irq(cfg->irq_mfcmcsc, vector_mfcmcsc_isr, 0, irq_name, vender);
		if (ret) {
			err_vec("failed to register isr for %d", PDEV_IRQ_NUM_MFCMCSC);
			return ret;
		}
	}
#endif
#endif

	/* 3AA 0 */
	snprintf(irq_name, SIZE_OF_NAME, "fimc-psv-%d", PDEV_IRQ_NUM_3AAW_0);
	ret = request_irq(cfg->irq_3aa0, vector_3aa0_isr, 0, irq_name, vender);
	if (ret) {
		err_vec("failed to register isr for %d", PDEV_IRQ_NUM_3AAW_0);
		return ret;
	}

	/* MC-Scaler 0 */
	snprintf(irq_name, SIZE_OF_NAME, "fimc-psv-%d", PDEV_IRQ_NUM_MCSC_0);
	ret = request_irq(cfg->irq_mcsc0, vector_mcsc0_isr, 0, irq_name, vender);
	if (ret) {
		err_vec("failed to register isr for %d", PDEV_IRQ_NUM_MCSC_0);
		return ret;
	}

	/* MC-Scaler 1 */
	snprintf(irq_name, SIZE_OF_NAME, "fimc-psv-%d", PDEV_IRQ_NUM_MCSC_1);
	ret = request_irq(cfg->irq_mcsc1, vector_mcsc1_isr, 0, irq_name, vender);
	if (ret) {
		err_vec("failed to register isr for %d", PDEV_IRQ_NUM_MCSC_1);
		return ret;
	}

	/* ISP 1 */
	snprintf(irq_name, SIZE_OF_NAME, "fimc-psv-%d", PDEV_IRQ_NUM_ISPHQ_0);
	ret = request_irq(cfg->irq_isp1, vector_isp1_isr, 0, irq_name, vender);
	if (ret) {
		err_vec("failed to register isr for %d", PDEV_IRQ_NUM_ISPHQ_0);
		return ret;
	}

	return 0;
}

static void fimc_is_vector_free_irqs(struct fimc_is_vender *vender)
{
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	/* free_irq(cfg->irq_mcsc, vender);
	free_irq(cfg->irq_vra0, vender); */
	free_irq(cfg->irq_isp0, vender);
#ifndef CONFIG_SOC_EXYNOS7570
	free_irq(cfg->irq_3aa0, vender);
	free_irq(cfg->irq_mcsc0, vender);
	free_irq(cfg->irq_vra1, vender);
	if (cfg->item.no_fimcis)
		free_irq(cfg->irq_mcsc0, vender);
#endif
	free_irq(cfg->irq_mcsc1, vender);
	free_irq(cfg->irq_isp1, vender);
}

static int fimc_is_isp_wait_done(struct fimc_is_vender *vender)
{
	int ret = 0;
	long remain;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	remain = wait_event_timeout(cfg->wait, atomic_read(&cfg->done_isp0),
						msecs_to_jiffies(cfg->item.timeout));

	if (!remain) {
		err_vec("vector execution timer is expired");
		ret = -ETIME;
	} else {
		dbg_vec("time to be done: %d\n", jiffies_to_msecs(remain));
	}

	return ret;
}

#if 0
static int fimc_is_3aaw_wait_done(struct fimc_is_vender *vender)
{
	int ret = 0;
	long remain;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	remain = wait_event_timeout(cfg->wait, atomic_read(&cfg->done_3aa0),
			msecs_to_jiffies(cfg->item.timeout));

	if (!remain) {
		err_vec("vector execution timer is expired");
		ret = -ETIME;
	} else {
		dbg_vec("time to be done: %d\n", jiffies_to_msecs(remain));
	}

	return ret;
}
#endif

#ifndef CONFIG_SOC_EXYNOS7570
static int fimc_is_3aa0_wait_done(struct fimc_is_vender *vender)
{
	int ret = 0;
	long remain;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	remain = wait_event_timeout(cfg->wait, atomic_read(&cfg->done_3aa0),
						msecs_to_jiffies(cfg->item.timeout));

	if (!remain) {
		err_vec("vector execution timer is expired");
		ret = -ETIME;
	} else {
		dbg_vec("time to be done: %d\n", jiffies_to_msecs(remain));
	}

	return ret;
}
#endif

#ifdef CONFIG_SOC_EXYNOS7570
static int fimc_is_vra1_wait_done(struct fimc_is_vender *vender)
{
	int ret = 0;
	long remain;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	remain = wait_event_timeout(cfg->wait, atomic_read(&cfg->vra1done),
						msecs_to_jiffies(cfg->item.timeout));

	if (!remain) {
		err_vec("vector execution timer is expired");
		ret = -ETIME;
	} else {
		dbg_vec("time to be done: %d\n", jiffies_to_msecs(remain));
	}

	return ret;
}
#endif

static int fimc_is_mfcmcsc_wait_done(struct fimc_is_vender *vender)
{
	int ret = 0;
	long remain;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	remain = wait_event_timeout(cfg->wait, atomic_read(&cfg->done_mcsc1),
						msecs_to_jiffies(cfg->item.timeout));

	if (!remain) {
		err_vec("vector execution timer is expired");
		ret = -ETIME;
	} else {
		dbg_vec("time to be done: %d\n", jiffies_to_msecs(remain));
	}

	return ret;
}

#if 0
static int fimc_is_mcsc_wait_done(struct fimc_is_vender *vender)
{
	int ret = 0;
	long remain;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	remain = wait_event_timeout(cfg->wait, atomic_read(&cfg->mcscdone),
						msecs_to_jiffies(cfg->item.timeout));

	if (!remain) {
		err_vec("vector execution timer is expired");
		ret = -ETIME;
	} else {
		dbg_vec("time to be done: %d\n", jiffies_to_msecs(remain));
	}

	return ret;
}
#endif

static int fimc_is_mcsc0_wait_done(struct fimc_is_vender *vender)
{
	int ret = 0;
	long remain;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	remain = wait_event_timeout(cfg->wait, atomic_read(&cfg->done_mcsc0),
						msecs_to_jiffies(cfg->item.timeout));

	if (!remain) {
		err_vec("vector execution timer is expired");
		ret = -ETIME;
	} else {
		dbg_vec("time to be done: %d\n", jiffies_to_msecs(remain));
	}

	return ret;
}

static int fimc_is_mcsc1_wait_done(struct fimc_is_vender *vender)
{
	int ret = 0;
	long remain;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	remain = wait_event_timeout(cfg->wait, atomic_read(&cfg->done_mcsc1),
						msecs_to_jiffies(cfg->item.timeout));

	if (!remain) {
		err_vec("vector execution timer is expired");
		ret = -ETIME;
	} else {
		dbg_vec("time to be done: %d\n", jiffies_to_msecs(remain));
	}

	return ret;
}

static int fimc_is_isp1_wait_done(struct fimc_is_vender *vender)
{
	int ret = 0;
	long remain;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	remain = wait_event_timeout(cfg->wait, atomic_read(&cfg->done_isp1),
						msecs_to_jiffies(cfg->item.timeout));

	if (!remain) {
		err_vec("vector execution timer is expired");
		ret = -ETIME;
	} else {
		dbg_vec("time to be done: %d\n", jiffies_to_msecs(remain));
	}

	return ret;
}

#if defined(CONFIG_SOC_EXYNOS7880)
static int fimcis_reset_7880(void)
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
	info_vec("csis0 reset...\n");

	/* csis1 */
	baseaddr = ioremap(0x14460000, 0x10000);
	__raw_writel(0x2, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if (readval == 0x00004000)
			break;
	}
	iounmap(baseaddr);
	info_vec("csis1 reset...\n");

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
	info_vec("bns reset...\n");

	/* 3aa */
	baseaddr = ioremap(0x14480000, 0x10000);
	__raw_writel(0x00000001, baseaddr + 0xC);
	while (1) {
		readval = __raw_readl(baseaddr + 0xC);
		if (readval  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_vec("3aa reset...\n");

	/* isp */
	baseaddr = ioremap(0x14400000, 0x10000);
	__raw_writel(0x00000001, baseaddr + 0xC);
	while (1) {
		readval = __raw_readl(baseaddr + 0xC);
		if (readval  == 0x00000000)
			break;
	}
	iounmap(baseaddr);
	info_vec("isp reset...\n");

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
	info_vec("mcsc reset...\n");

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
	info_vec("vra reset...\n");

	return 0;
}

#elif defined(CONFIG_SOC_EXYNOS7570)
static int fimcis_reset_7570(void)
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
	info_vec("csis0 reset...\n");

	/* bsc */
	baseaddr = ioremap(0x14410000, 0x10000);
	__raw_writel(0x00020000, baseaddr + 0x4);
	while (1) {
		readval = __raw_readl(baseaddr + 4);
		if (readval == 0x00040000)
			break;
	}
	iounmap(baseaddr);
	info_vec("bsc reset...\n");

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
	info_vec("isp reset...\n");

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
	info_vec("mcsc reset...\n");

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
	info_vec("vra reset...\n");

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
	info_vec("mfc_mcsc reset...\n");

	return 0;
}

#elif defined(CONFIG_SOC_EXYNOS8895)
static int fimcis_reset_8895(void)
{
	void __iomem *baseaddr;
	u32 readval;
	int sfr_offs = 0;
	u32 sysmmu_cam0;
	u32 sysmmu_cam1;
	u32 sysmmu_isp;
	u32 sysmmu_srdz;

	baseaddr = ioremap(0x12D10000, 0x10000);
	sysmmu_cam0 = __raw_readl(baseaddr);
	__raw_writel(0x00000000, baseaddr);
	iounmap(baseaddr);

	baseaddr = ioremap(0x12D20000, 0x10000);
	sysmmu_cam1 = __raw_readl(baseaddr);
	__raw_writel(0x00000000, baseaddr);
	iounmap(baseaddr);

	baseaddr = ioremap(0x13050000, 0x10000);
	sysmmu_isp = __raw_readl(baseaddr);
	__raw_writel(0x00000000, baseaddr);
	iounmap(baseaddr);

	baseaddr = ioremap(0x14230000, 0x10000);
	sysmmu_srdz = __raw_readl(baseaddr);
	__raw_writel(0x00000000, baseaddr);
	iounmap(baseaddr);

#if 0
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
#endif

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

	baseaddr = ioremap(0x12D10000, 0x10000);
	__raw_writel(sysmmu_cam0, baseaddr);
	iounmap(baseaddr);

	baseaddr = ioremap(0x12D20000, 0x10000);
	__raw_writel(sysmmu_cam1, baseaddr);
	iounmap(baseaddr);

	baseaddr = ioremap(0x13050000, 0x10000);
	__raw_writel(sysmmu_isp, baseaddr);
	iounmap(baseaddr);

	baseaddr = ioremap(0x14230000, 0x10000);
	__raw_writel(sysmmu_srdz, baseaddr);
	iounmap(baseaddr);

	return 0;
}
#endif

static int fimcis_reset(void)
{
	int errorcode = 0;

#if defined(CONFIG_SOC_EXYNOS7880)
	errorcode = fimcis_reset_7880();
#elif defined(CONFIG_SOC_EXYNOS7570)
	errorcode = fimcis_reset_7570();
#elif defined(CONFIG_SOC_EXYNOS8895)
	errorcode = fimcis_reset_8895();
#else
	errorcode = 0xFFFF;
	err_vec("Err: check project ID: %x\n", errorcode);
#endif

	return errorcode;
}

#undef USE_CRC_ADDR /* ? SFR for CRC uses absolute address or offset */
#ifdef USE_CRC_ADDR
#define CRC_EXTRA_OFS	0x14000000
#else
#define CRC_EXTRA_OFS	0
#endif
static int fimc_is_vector_check_crc(struct fimc_is_vender *vender, int id)
{
	int ret = 0;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	struct vector_crc *crc;
	unsigned int result;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	list_for_each_entry(crc, &cfg->crc, list) {
		dbg_vec("checking CRC addr: 0x%08x, value: 0x%08x, mask: 0x%08x\n",
				crc->sfr_addr, crc->value, crc->sfr_mask);

		if (cfg->item.no_fimcis)
			result = __raw_readl(cfg->baseaddr + crc->sfr_addr);
		else
			result = vector_readl(cfg->baseaddr, crc->sfr_addr - CRC_EXTRA_OFS);
		result = result & crc->sfr_mask;
		if (result != crc->value) {
			err_vec("CRC is mismatched at addr: 0x%08x\n"
					"\t\t\t\texpect: 0x%08x, result: 0x%08x",
					crc->sfr_addr, crc->value, result);
			ret = -EFAULT;
		} else {
			dbg_vec("CRC is matched at addr: 0x%08x\n"
					"\t\t\t\texpect: 0x%08x, result: 0x%08x\n",
					crc->sfr_addr, crc->value, result);
		}
	}

	return ret;
}

static int fimc_is_vector_check_chksum(struct fimc_is_vender *vender, int id)
{
	int ret = 0;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	struct vector_dma *dma;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	list_for_each_entry(dma, &cfg->dma, list) {
#ifdef INPUT_CHKSUM
		if (dma->dir == DMA_DIR_INPUT)
			dbg_vec("checksum for DMA[I] ofs: 0x%08x, size: 0x%08x, "
					" value: 0x%016lx\n",
					dma->sfr_ofs, dma->size, dma->chksum_input);
#endif
		if (dma->dir == DMA_DIR_OUTPUT) {
			dbg_vec("making checksum for DMA ofs: 0x%08x, size: 0x%08x\n",
					dma->sfr_ofs, dma->size);

			CALL_VOID_BUFOP(dma->pbuf, sync_for_cpu, dma->pbuf,
					0, dma->size, DMA_FROM_DEVICE);
			dma->chksum_result = make_chksum_64(
					(void *)CALL_BUFOP(dma->pbuf, kvaddr, dma->pbuf),
					dma->size);

			if (dma->chksum_result != dma->chksum_expect) {
				err_vec("checksum for DMA[O] ofs: 0x%08x, size: 0x%08x\n"
						"\t\t\t\texpected: 0x%016lx, result: 0x%016lx",
						dma->sfr_ofs, dma->size,
						dma->chksum_expect, dma->chksum_result);
				ret = -EFAULT;
			} else {
				dbg_vec("checksum for DMA[O] ofs: 0x%08x, size: 0x%08x\n"
						"\t\t\t\texpected: 0x%016lx, result: 0x%016lx\n",
						dma->sfr_ofs, dma->size,
						dma->chksum_expect, dma->chksum_result);
			}
		}
	}

	return ret;
}

int fimc_is_vector_set(struct fimc_is_core *core, int id)
{
	struct fimc_is_vender *vender = &core->vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	int ret;
	unsigned long nanosec_rem;
	unsigned long long time_diff;
	int ip_num;
#if 0
	unsigned int all, enable, status;
#endif
	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;


	cfg->measure_time_enable = 0;

	/* TODO: multiple frame */
	/* TODO: hook origianl ISRs */

	fimcis_reset();

	ret = fimc_is_vector_cfg_load(vender, id);
	if (ret) {
		err_vec("failed to load configuration for vector%d", id);
		return ret;
	}

	ret = fimc_is_vector_dma_load(vender, id);
	if (ret) {
		err_vec("failed to load DMA configuration for vector%d", id);
		return ret;
	}

	ret = fimc_is_vector_crc_load(vender, id);
	if (ret) {
		err_vec("failed to load CRC configuration for vector%d", id);
		goto err_crc_load;
	}

	ret = fimc_is_vector_load_n_set(vender, id, "sfr");
	if (ret) {
		err_vec("failed to set %s configuration %d", "sfr", id);
		goto err_load_n_set_sfr;
	}

	fimc_is_vector_dma_set(vender, id);

	ret = fimc_is_vector_sysmmu_resume(vender);
	if (ret) {
		err_vec("failed to resume SYS.MMU for vector%d", id);
		goto err_sysmmu_resume;
	}

	/*  modified to use irq always    */
	ret = fimc_is_vector_request_irqs(vender);
	if (ret) {
		err_vec("failed to request IRQs vector%d", id);
		goto err_req_irqs;
	}

	ret = fimc_is_vector_load_n_set(vender, id, "enable");
	if (ret) {
		err_vec("failed to set %s configuration for vector%d", "enable", id);
		goto err_load_n_set_enable;
	}

#if 0
	while (0) {
		all = vector_readl(cfg->baseaddr, 0x13030104);
		enable = vector_readl(cfg->baseaddr, 0x13030108);
		status = vector_readl(cfg->baseaddr, 0x1303010c);
		pr_info("3AAW INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
				all, enable, status);
		vector_writel(status, cfg->baseaddr, 0x13030110);

		all = vector_readl(cfg->baseaddr, 0x13130104);
		enable = vector_readl(cfg->baseaddr, 0x13130108);
		status = vector_readl(cfg->baseaddr, 0x1313010c);
		pr_info("3AA INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
				all, enable, status);
		vector_writel(status, cfg->baseaddr, 0x13130110);

		all = vector_readl(cfg->baseaddr, 0x13042604);
		enable = vector_readl(cfg->baseaddr, 0x13042608);
		status = vector_readl(cfg->baseaddr, 0x1304260c);
		pr_info("ISPLP INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
				all, enable, status);
		vector_writel(status, cfg->baseaddr, 0x13042610);

		all = vector_readl(cfg->baseaddr, 0x13143504);
		enable = vector_readl(cfg->baseaddr, 0x13143508);
		status = vector_readl(cfg->baseaddr, 0x1314350c);
		pr_info("ISPHQ INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
				all, enable, status);
		vector_writel(status, cfg->baseaddr, 0x13143510);

		all = vector_readl(cfg->baseaddr, 0x12c40204);
		enable = vector_readl(cfg->baseaddr, 0x12c40208);
		status = vector_readl(cfg->baseaddr, 0x12c4020c);
		pr_info("TPU0 INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
				all, enable, status);
		vector_writel(status, cfg->baseaddr, 0x12c40210);

		all = vector_readl(cfg->baseaddr, 0x12c90204);
		enable = vector_readl(cfg->baseaddr, 0x12c90208);
		status = vector_readl(cfg->baseaddr, 0x12c9020c);
		pr_info("TPU1 INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
				all, enable, status);
		vector_writel(status, cfg->baseaddr, 0x12c90210);

		usleep_range(1000, 1000);
	}
#endif

	if (!cfg->item.sync) {
		cfg->async_id = id;
		return 0;
	}

	if (!cfg->item.no_fimcis) {
#ifdef CONFIG_SOC_EXYNOS8895
		/*TODO: Below code can not cover the ASB scn002. */
		ret = fimc_is_mcsc0_wait_done(vender);
		if (ret)
			err_vec("error is occurred while waiting MCSC0 vector%d", id);

		ret = fimc_is_mcsc1_wait_done(vender);
		if (ret)
			err_vec("error is occurred while waiting MCSC1 vector%d", id);

		ret = fimc_is_isp1_wait_done(vender);
		if (ret)
			err_vec("error is occurred while waiting ISP1 vector%d", id);

#else
#ifndef CONFIG_SOC_EXYNOS7570
		ret = fimc_is_3aa0_wait_done(vender);
		if (ret)
			err_vec("error is occurred while waiting 3aadone vector%d", id);
#endif

		ret = fimc_is_isp_wait_done(vender);
		if (ret)
			err_vec("error is occurred while waiting ispdone vector%d", id);

#ifdef CONFIG_SOC_EXYNOS7570
		ret = fimc_is_vra1_wait_done(vender);
		if (ret)
			err_vec("error is occurred while waiting vra1done vector%d", id);
#endif
#endif
	}
	else {
		ret = fimc_is_mfcmcsc_wait_done(vender);
		if (ret)
			err_vec("error is occurred while waiting done_mcsc1 vector%d", id);
	}

#if 0
	usleep_range(500000, 500000);

	all = vector_readl(cfg->baseaddr, 0x13030104);
	enable = vector_readl(cfg->baseaddr, 0x13030108);
	status = vector_readl(cfg->baseaddr, 0x1303010c);
	pr_info("3AAW INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
			all, enable, status);
	vector_writel(status, cfg->baseaddr, 0x13030110);

	all = vector_readl(cfg->baseaddr, 0x13130104);
	enable = vector_readl(cfg->baseaddr, 0x13130108);
	status = vector_readl(cfg->baseaddr, 0x1313010c);
	pr_info("3AA INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
			all, enable, status);
	vector_writel(status, cfg->baseaddr, 0x13130110);

	all = vector_readl(cfg->baseaddr, 0x13042604);
	enable = vector_readl(cfg->baseaddr, 0x13042608);
	status = vector_readl(cfg->baseaddr, 0x1304260c);
	pr_info("ISPLP INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
			all, enable, status);
	vector_writel(status, cfg->baseaddr, 0x13042610);

	all = vector_readl(cfg->baseaddr, 0x13143504);
	enable = vector_readl(cfg->baseaddr, 0x13143508);
	status = vector_readl(cfg->baseaddr, 0x1314350c);
	pr_info("ISPHQ INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
			all, enable, status);
	vector_writel(status, cfg->baseaddr, 0x13143510);

	all = vector_readl(cfg->baseaddr, 0x12c40204);
	enable = vector_readl(cfg->baseaddr, 0x12c40208);
	status = vector_readl(cfg->baseaddr, 0x12c4020c);
	pr_info("TPU0 INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
			all, enable, status);
	vector_writel(status, cfg->baseaddr, 0x12c40210);

	all = vector_readl(cfg->baseaddr, 0x12c90204);
	enable = vector_readl(cfg->baseaddr, 0x12c90208);
	status = vector_readl(cfg->baseaddr, 0x12c9020c);
	pr_info("TPU1 INT all: 0x%08x, enable: 0x%08x, status: 0x%08x\n",
			all, enable, status);
	vector_writel(status, cfg->baseaddr, 0x12c90210);

	status = vector_readl(cfg->baseaddr, 0x13132504);
	pr_info("3AA DMA client status: 0x%08x\n", status);
#endif

	ret = fimc_is_vector_load_n_set(vender, id, "disable");
	if (ret)
		err_vec("failed to set %s configuration vector%d", "disable", id);

	if (cfg->measure_time_enable) {
		for (ip_num = 0; ip_num < MAX_PDEV_IRQ_NUM; ip_num++) {
			time_diff = cfg->time[ip_num].end_time - cfg->time[ip_num].start_time;
			nanosec_rem = do_div(time_diff, NSEC_PER_SEC);
			info("IP[%d] time %lu ns\n", ip_num, nanosec_rem);
		}
	}

	/* CRC */
	if (cfg->item.verification & VERIFICATION_CRC) {
		ret = fimc_is_vector_check_crc(vender, id);
		if (ret) {
			err_vec("CRC mismatch(es) were occured vector%d", id);
			goto err_load_n_set_enable;
		}
	}

	pr_info("before checksum: 0x%08x\n", cfg->item.verification);
	/* checksum */
	if (cfg->item.verification & VERIFICATION_CHKSUM) {
		ret = fimc_is_vector_check_chksum(vender, id);
		if (ret) {
			err_vec("checksum mismatch(es) were occured vector%d", id);
			goto err_load_n_set_enable;
		}
	}

	if (cfg->item.dump_dma) {
		if (fimc_is_vector_dma_dump(vender, id))
			err_vec("failed to dump DMA vector%d", id);
	}

err_load_n_set_enable:
	fimc_is_vector_free_irqs(vender);

err_req_irqs:
	fimc_is_vector_sysmmu_suspend(vender);

err_sysmmu_resume:
err_load_n_set_sfr:
	__flush_crc_cfg_entries(cfg);

err_crc_load:
	__flush_dma_cfg_entries(cfg);

	fimc_is_vector_cfg_unload(vender, id);

	/* TODO: restore origianl ISRs */

	return ret;
}

int fimc_is_vector_get(struct fimc_is_core *core)
{
	struct fimc_is_vender *vender = &core->vender;
	struct fimc_is_vender_specific *priv;
	struct vector_cfg *cfg;
	int ret;
	int id;

	priv = (struct fimc_is_vender_specific *)vender->private_data;
	cfg = &priv->vector_cfg;

	if (cfg->async_id == -1) {
		err_vec("vector lastest was executed was sync mode");
		return -EINVAL;
	}
	id = cfg->async_id;

#ifndef CONFIG_SOC_EXYNOS7570
	ret = fimc_is_3aa0_wait_done(vender);
	if (ret)
		err_vec("error is occurred while waiting 3aadone vector%d", id);
#endif

	ret = fimc_is_isp_wait_done(vender);
	if (ret)
		err_vec("error is occurred while waiting ispdone vector%d", id);

	ret = fimc_is_vector_load_n_set(vender, id, "disable");
	if (ret)
		err_vec("failed to set %s configuration vector%d", "disable", id);

	/* CRC */
	if (cfg->item.verification & VERIFICATION_CRC) {
		ret = fimc_is_vector_check_crc(vender, id);
		if (ret) {
			err_vec("CRC mismatch(es) were occured vector%d", id);
			goto p_err;
		}
	}

	/* checksum */
	if (cfg->item.verification & VERIFICATION_CHKSUM) {
		ret = fimc_is_vector_check_chksum(vender, id);
		if (ret) {
			err_vec("checksum mismatch(es) were occured vector%d", id);
			goto p_err;
		}
	}

	if (cfg->item.dump_dma) {
		if (fimc_is_vector_dma_dump(vender, id))
			err_vec("failed to dump DMA vector%d", id);
	}

p_err:
	fimc_is_vector_free_irqs(vender);

	//fimcis_reset();

	fimc_is_vector_sysmmu_suspend(vender);

	__flush_crc_cfg_entries(cfg);

	__flush_dma_cfg_entries(cfg);

	fimc_is_vector_cfg_unload(vender, id);

	/* TODO: restore origianl ISRs */

	return ret;
}
