/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2021 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#ifndef __extlinux_h
#define __extlinux_h

#include <pxe_utils.h>

#define EXTLINUX_FNAME	"extlinux/extlinux.conf"

/**
 * struct extlinux_info - useful information for extlinux_getfile()
 *
 * @dev: bootmethod device being used to boot
 * @bflow: bootflow being booted
 */
struct extlinux_info {
	struct udevice *dev;
	struct bootflow *bflow;
};

/**
 * struct extlinux_plat - locate state for this bootmeth
 *
 * @use_falllback: true to boot with the fallback option
 * @ctx: holds the PXE context, if it should be saved
 * @info: information used for the getfile() method
 */
struct extlinux_plat {
	bool use_fallback;
	struct pxe_context ctx;
	struct extlinux_info info;
};

/**
 * extlinux_set_property() - set an extlinux property
 *
 * This allows the setting of bootmeth-specific properties to enable
 * automated finer-grained control of the boot process
 *
 * @name: String containing the name of the relevant boot method
 * @property: String containing the name of the property to set
 * @value: String containing the value to be set for the specified property
 * Return: 0 if OK, -EINVAL if an unknown property or invalid value is provided
 */
int extlinux_set_property(struct udevice *dev, const char *property,
			  const char *value);

/**
 * extlinux_boot() - Boot a bootflow
 *
 * @dev: bootmeth device
 * @bflow: Bootflow to boot
 * @getfile: Function to use to read files
 * @allow_abs_path: true to allow absolute paths
 * @bootfile: Bootfile whose directory loaded files are relative to, NULL if
 *	none
 * Return: 0 if OK, -ve error code on failure
 */
int extlinux_boot(struct udevice *dev, struct bootflow *bflow,
		  pxe_getfile_func getfile, bool allow_abs_path,
		  const char *bootfile);

/**
 * extlinux_read_all() - read all files for a bootflow
 *
 * @dev: Bootmethod device to boot
 * @bflow: Bootflow to read
 * @getfile: Function to use to read files
 * @allow_abs_path: true to allow absolute paths
 * @bootfile: Bootfile whose directory loaded files are relative to, NULL if
 *	none
 * Return: 0 if OK, -EIO on I/O error, other -ve on other error
 */
int extlinux_read_all(struct udevice *dev, struct bootflow *bflow,
		      pxe_getfile_func getfile, bool allow_abs_path,
		      const char *bootfile);

#endif
