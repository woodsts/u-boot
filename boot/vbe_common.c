// SPDX-License-Identifier: GPL-2.0
/*
 * Verified Boot for Embedded (VBE) common functions
 *
 * Copyright 2024 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <blk.h>
#include <bootstage.h>
#include <display_options.h>
#include <dm.h>
#include <image.h>
#include <spl.h>
#include <mapmem.h>
#include <memalign.h>
#include <linux/types.h>
#include <u-boot/crc.h>
#include "vbe_common.h"

int vbe_read_version(struct udevice *blk, ulong offset, char *version,
		     int max_size)
{
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, MMC_MAX_BLOCK_LEN);

	if (max_size > MMC_MAX_BLOCK_LEN)
		return log_msg_ret("ver", -E2BIG);

	if (offset & (MMC_MAX_BLOCK_LEN - 1))
		return log_msg_ret("get", -EBADF);
	offset /= MMC_MAX_BLOCK_LEN;

	if (blk_read(blk, offset, 1, buf) != 1)
		return log_msg_ret("read", -EIO);
	strlcpy(version, buf, max_size);

	return 0;
}

int vbe_read_nvdata(struct udevice *blk, ulong offset, ulong size, u8 *buf)
{
	uint hdr_ver, hdr_size, data_size, crc;
	const struct vbe_nvdata *nvd;

	if (size > MMC_MAX_BLOCK_LEN)
		return log_msg_ret("state", -E2BIG);

	if (offset & (MMC_MAX_BLOCK_LEN - 1))
		return log_msg_ret("get", -EBADF);
	offset /= MMC_MAX_BLOCK_LEN;

	if (blk_read(blk, offset, 1, buf) != 1)
		return log_msg_ret("read", -EIO);
	nvd = (struct vbe_nvdata *)buf;
	hdr_ver = (nvd->hdr & NVD_HDR_VER_MASK) >> NVD_HDR_VER_SHIFT;
	hdr_size = (nvd->hdr & NVD_HDR_SIZE_MASK) >> NVD_HDR_SIZE_SHIFT;
	if (hdr_ver != NVD_HDR_VER_CUR)
		return log_msg_ret("hdr", -EPERM);
	data_size = 1 << hdr_size;
	if (data_size > sizeof(*nvd))
		return log_msg_ret("sz", -EPERM);

	crc = crc8(0, buf + 1, data_size - 1);
	if (crc != nvd->crc8)
		return log_msg_ret("crc", -EPERM);

	return 0;
}

int vbe_get_blk(const char *storage, struct udevice **blkp)
{
	struct blk_desc *desc;
	char devname[16];
	const char *end;
	int devnum;

	/* First figure out the block device */
	log_debug("storage=%s\n", storage);
	devnum = trailing_strtoln_end(storage, NULL, &end);
	if (devnum == -1)
		return log_msg_ret("num", -ENODEV);
	if (end - storage >= sizeof(devname))
		return log_msg_ret("end", -E2BIG);
	strlcpy(devname, storage, end - storage + 1);
	log_debug("dev=%s, %x\n", devname, devnum);

	desc = blk_get_dev(devname, devnum);
	if (!desc)
		return log_msg_ret("get", -ENXIO);
	*blkp = desc->bdev;

	return 0;
}
