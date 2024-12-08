// SPDX-License-Identifier: GPL-2.0+
/*
 * Aurora Innovation, Inc. Copyright 2022.
 *
 */

#define __efi_runtime

#include <errno.h>
#include <asm/global_data.h>
#include <efi.h>
#include <efi_api.h>
#include <efi_variable.h>

DECLARE_GLOBAL_DATA_PTR;

efi_status_t efi_get_variable_int(const u16 *variable_name, const efi_guid_t *vendor,
				  u32 *attributes, efi_uintn_t *data_size,
				  void *data, u64 *timep)
{
	struct efi_priv *priv = efi_get_priv();
	struct efi_runtime_services *run = priv->run;

	return run->get_variable((u16 *)variable_name, vendor, attributes, data_size, data);
}

efi_status_t efi_set_variable_int(const u16 *variable_name, const efi_guid_t *vendor,
				  u32 attributes, efi_uintn_t data_size, const void *data,
				  bool ro_check)
{
	struct efi_priv *priv = efi_get_priv();
	struct efi_runtime_services *run = priv->run;

	return run->set_variable((u16 *)variable_name, vendor, attributes, data_size, data);
}

efi_status_t efi_get_next_variable_name_int(efi_uintn_t *variable_name_size,
					    u16 *variable_name, efi_guid_t *vendor)
{
	struct efi_priv *priv = efi_get_priv();
	struct efi_runtime_services *run = priv->run;

	return run->get_next_variable_name(variable_name_size, variable_name, vendor);
}
