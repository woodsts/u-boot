// SPDX-License-Identifier: GPL-2.0+
/*
 * Aurora Innovation, Inc. Copyright 2022.
 *
 */

#include <blk.h>
#include <config.h>
#include <command.h>
#include <env.h>
#include <fs.h>
#include <lmb.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

int do_addr_find(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct lmb_region *mem, *reserved;
	const char *filename;
	struct lmb lmb;
	loff_t size;
	int ret;
	int i, j;

	if (!gd->fdt_blob) {
		log_err("No FDT setup\n");
		return CMD_RET_FAILURE;
	}

	if (fs_set_blk_dev(argv[1], argc >= 3 ? argv[2] : NULL, FS_TYPE_FAT)) {
		log_err("Can't set block device\n");
		return CMD_RET_FAILURE;
	}

	if (argc >= 4) {
		filename = argv[3];
	} else {
		filename = env_get("bootfile");
		if (!filename) {
			log_err("No boot file defined\n");
			return CMD_RET_FAILURE;
		}
	}

	ret = fs_size(filename, &size);
	if (ret != 0) {
		log_err("Failed to get file size\n");
		return CMD_RET_FAILURE;
	}

	lmb_init_and_reserve(&lmb, gd->bd, (void *)gd->fdt_blob);
	mem = &lmb.memory;
	reserved = &lmb.reserved;

	for (i = 0; i < mem->cnt; i++) {
		unsigned long long start, end;

		start = mem->region[i].base;
		end = mem->region[i].base + mem->region[i].size - 1;
		if ((start + size) > end)
			continue;
		for (j = 0; j < reserved->cnt; j++) {
			if ((reserved->region[j].base + reserved->region[j].size) < start)
				continue;
			if ((start + size) > reserved->region[j].base)
				start = reserved->region[j].base + reserved->region[j].size;
		}
		if ((start + size) <= end) {
			env_set_hex("loadaddr", start);
			debug("Set loadaddr to 0x%llx\n", start);
			return CMD_RET_SUCCESS;
		}
	}

	log_err("Failed to find enough RAM for 0x%llx bytes\n", size);
	return CMD_RET_FAILURE;
}

U_BOOT_CMD(
	addr_find, 7, 1, do_addr_find,
	"find a load address suitable for a file",
	"<interface> [<dev[:part]>] <filename>\n"
	"- find a consecutive region of memory sufficiently large to hold\n"
	"  the file called 'filename' from 'dev' on 'interface'. If\n"
	"  successful, 'loadaddr' will be set to the located address."
);
