// SPDX-License-Identifier: GPL-2.0+
/*
 * Bootmethod for sunxi FEL loading
 *
 * Copyright 2024 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#define LOG_CATEGORY UCLASS_BOOTSTD

#include <bootdev.h>
#include <bootflow.h>
#include <bootmeth.h>
#include <command.h>
#include <dm.h>
#include <env.h>

static int fel_check(struct udevice *dev, struct bootflow_iter *iter)
{
	return 0;
}

static int fel_read_bootflow(struct udevice *dev, struct bootflow *bflow)
{
	if (!env_get("fel_booted") || !env_get("fel_scriptaddr"))
		return -ENOENT;

	bflow->name = strdup("fel-script");
	if (!bflow->name)
		return log_msg_ret("fel", -ENOMEM);
	bflow->state = BOOTFLOWST_READY;

	return 0;
}

static int fel_read_file(struct udevice *dev, struct bootflow *bflow,
			 const char *file_path, ulong addr,
			 enum bootflow_img_t type, ulong *sizep)
{
	return -ENOSYS;
}

static int fel_boot(struct udevice *dev, struct bootflow *bflow)
{
	ulong addr;
	int ret;

	addr = env_get_hex("fel_scriptaddr", 0);
	ret = cmd_source_script(addr, NULL, NULL);
	if (ret)
		return log_msg_ret("boot", ret);

	return 0;
}

static int fel_bootmeth_bind(struct udevice *dev)
{
	struct bootmeth_uc_plat *plat = dev_get_uclass_plat(dev);

	plat->desc = IS_ENABLED(CONFIG_BOOTSTD_FULL) ?
	   "Sunxi FEL boot over USB" : "FEL";
	plat->flags = BOOTMETHF_GLOBAL;

	return 0;
}

static struct bootmeth_ops fel_bootmeth_ops = {
	.check		= fel_check,
	.read_bootflow	= fel_read_bootflow,
	.read_file	= fel_read_file,
	.boot		= fel_boot,
};

static const struct udevice_id fel_bootmeth_ids[] = {
	{ .compatible = "u-boot,fel-bootmeth" },
	{ }
};

U_BOOT_DRIVER(bootmeth_2fel) = {
	.name		= "bootmeth_fel",
	.id		= UCLASS_BOOTMETH,
	.of_match	= fel_bootmeth_ids,
	.ops		= &fel_bootmeth_ops,
	.bind		= fel_bootmeth_bind,
};
