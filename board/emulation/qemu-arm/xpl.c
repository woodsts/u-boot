// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#define LOG_DEBUG
#define LOG_CATEGORY	LOGC_BOOT

#include <mapmem.h>
#include <spl.h>

unsigned int spl_boot_device(void)
{
	return BOOT_DEVICE_BOARD;
}

static int binman_load_image(struct spl_image_info *img,
			     struct spl_boot_device *bootdev)
{
	ulong base = spl_get_image_pos();
	ulong size = spl_get_image_size();
	ulong spl_load_addr;
	int ret;

	log_debug("Booting from address %lx size %lx\n", base, size);
	img->name = spl_phase_name(spl_next_phase());
	img->load_addr = base;
	img->entry_point = base;

	if (CONFIG_IS_ENABLED(RELOC_LOADER)) {
		img->size = size;
		spl_set_fdt_size(img, 0);
		ret = spl_reloc_prepare(img, &spl_load_addr);
		if (ret)
			return log_msg_ret("rel", ret);
		log_debug("Loading to %lx\n", spl_load_addr);
		memcpy(map_sysmem(spl_load_addr, size),
		       map_sysmem(base, size), size);
		img->load_addr = CONFIG_SPL_TEXT_BASE;
		img->entry_point = CONFIG_SPL_TEXT_BASE;
	}

	return 0;
}
SPL_LOAD_IMAGE_METHOD("binman", 0, BOOT_DEVICE_BOARD, binman_load_image);

void reset_cpu(void)
{
}
