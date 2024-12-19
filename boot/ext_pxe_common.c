// SPDX-License-Identifier: GPL-2.0+
/*
 * Common functions for extlinux and PXE
 *
 * Copyright 2024 Collabora
 * Written by Martyn Welch <martyn.welch@collabora.com>
 */

#define LOG_CATEGORY UCLASS_BOOTSTD

#include <dm.h>
#include <extlinux.h>
#include <mapmem.h>
#include <linux/string.h>

enum extlinux_option_type {
	EO_FALLBACK,
	EO_INVALID
};

struct extlinux_option {
	char *name;
	enum extlinux_option_type option;
};

static const struct extlinux_option options[] = {
	{"fallback", EO_FALLBACK},
	{NULL, EO_INVALID}
};

static enum extlinux_option_type extlinux_get_option(const char *option)
{
	int i = 0;

	while (options[i].name) {
		if (!strcmp(options[i].name, option))
			return options[i].option;

		i++;
	}

	return EO_INVALID;
};

int extlinux_set_property(struct udevice *dev, const char *property,
			  const char *value)
{
	struct extlinux_plat *plat;
	static enum extlinux_option_type option;

	plat = dev_get_plat(dev);

	option = extlinux_get_option(property);
	if (option == EO_INVALID) {
		printf("Invalid option\n");
		return -EINVAL;
	}

	switch (option) {
	case EO_FALLBACK:
		if (!strcmp(value, "1")) {
			plat->use_fallback = true;
		} else if (!strcmp(value, "0")) {
			plat->use_fallback = false;
		} else {
			printf("Unexpected value '%s'\n", value);
			return -EINVAL;
		}
		break;
	default:
		printf("Unrecognised property '%s'\n", property);
		return -EINVAL;
	}

	return 0;
}

static int extlinux_setup(struct udevice *dev, struct bootflow *bflow,
			  pxe_getfile_func getfile, bool allow_abs_path,
			  const char *bootfile, struct pxe_context *ctx)
{
	struct extlinux_plat *plat = dev_get_plat(dev);
	int ret;

	plat->info.dev = dev;
	plat->info.bflow = bflow;

	ret = pxe_setup_ctx(ctx, getfile, &plat->info, allow_abs_path, bootfile,
			    false, plat->use_fallback, bflow);
	if (ret)
		return log_msg_ret("ctx", ret);

	return 0;
}

int extlinux_boot(struct udevice *dev, struct bootflow *bflow,
		  pxe_getfile_func getfile, bool allow_abs_path,
		  const char *bootfile)
{
	struct extlinux_plat *plat = dev_get_plat(dev);
	ulong addr;
	int ret;

	/* if we have already selected a label, just boot it */
	if (plat->ctx.label) {
		ret = pxe_do_boot(&plat->ctx);
	} else {
		ret = extlinux_setup(dev, bflow, getfile, allow_abs_path,
				     bootfile, &plat->ctx);
		if (ret)
			return log_msg_ret("elb", ret);
		addr = map_to_sysmem(bflow->buf);
		ret = pxe_process(&plat->ctx, addr, false);
	}
	if (ret)
		return log_msg_ret("elb", -EFAULT);

	return 0;
}

int extlinux_read_all(struct udevice *dev, struct bootflow *bflow,
		      pxe_getfile_func getfile, bool allow_abs_path,
		      const char *bootfile)
{
	struct extlinux_plat *plat = dev_get_plat(dev);
	ulong addr;
	int ret;

	ret = extlinux_setup(dev, bflow, getfile, allow_abs_path, bootfile,
			     &plat->ctx);
	if (ret)
		return log_msg_ret("era", ret);
	addr = map_to_sysmem(bflow->buf);
	ret = pxe_probe(&plat->ctx, addr, false);
	if (ret)
		return log_msg_ret("elb", -EFAULT);

	return 0;
}
