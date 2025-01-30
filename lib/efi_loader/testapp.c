// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * EFI test application
 *
 * Copyright 2024 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 *
 * This test program is used to test the invocation of an EFI application.
 * It writes a few messages to the console and then exits boot services
 */

#include <efi_api.h>

static const efi_guid_t loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

static struct efi_system_table *systable;
static struct efi_boot_services *boottime;
static struct efi_simple_text_output_protocol *con_out;

/**
 * efi_main() - entry point of the EFI application.
 *
 * @handle:	handle of the loaded image
 * @systab:	system table
 * Return:	status code
 */
efi_status_t EFIAPI efi_main(efi_handle_t handle,
			     struct efi_system_table *systab)
{
	struct efi_loaded_image *loaded_image;
	struct efi_mem_desc *map;
	efi_status_t ret;
	efi_uintn_t map_size;
	efi_uintn_t map_key;
	efi_uintn_t desc_size;
	u32 desc_version;

	systable = systab;
	boottime = systable->boottime;
	con_out = systable->con_out;

	/* Get the loaded image protocol */
	ret = boottime->open_protocol(handle, &loaded_image_guid,
				      (void **)&loaded_image, NULL, NULL,
				      EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (ret != EFI_SUCCESS) {
		con_out->output_string
			(con_out, u"Cannot open loaded image protocol\r\n");
		goto out;
	}

	/* UEFI requires CR LF */
	con_out->output_string(con_out, u"U-Boot test app for EFI_LOADER\r\n");

out:
	map_size = 0;
	ret = boottime->get_memory_map(&map_size, NULL, &map_key, &desc_size,
				       &desc_version);
	if (ret != EFI_BUFFER_TOO_SMALL) {
		con_out->output_string(con_out, u"map error A\n");
		return ret;
	}

	ret = boottime->allocate_pool(EFI_BOOT_SERVICES_DATA, map_size,
				      (void **)&map);
	if (ret) {
		con_out->output_string(con_out, u"map error B\n");
		return ret;
	}
	/* Allocate extra space for newly allocated memory */
	map_size += sizeof(struct efi_mem_desc);
	ret = boottime->get_memory_map(&map_size, map, &map_key, &desc_size,
				       &desc_version);
	if (ret) {
		con_out->output_string(con_out, u"map error C\n");
		return ret;
	}

	/* exit boot services so that this part of U-Boot can be tested */
	con_out->output_string(con_out, u"Exiting boot services\n");
	ret = boottime->exit_boot_services(handle, map_key);
	if (ret) {
		con_out->output_string(con_out, u"Failed exit-boot-services\n");
		return ret;
	}

	/* now exit for real */
	con_out->output_string(con_out, u"Exiting test app\n");
	ret = boottime->exit(handle, ret, 0, NULL);

	/* We should never arrive here */
	return ret;
}
