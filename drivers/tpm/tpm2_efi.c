// SPDX-License-Identifier: GPL-2.0+
/*
 * Aurora Innovation, Inc. Copyright 2022.
 *
 */

#include <config.h>
#include <dm.h>
#include <efi.h>
#include <efi_api.h>
#include <efi_tcg2.h>
#include <malloc.h>
#include <tpm-v2.h>

static int efi_tpm_bind(struct udevice *dev)
{
	struct tpm_chip_priv *priv = dev_get_uclass_priv(dev);
	struct efi_tpm_plat *plat = dev_get_plat(dev);
	struct efi_tcg2_boot_service_capability caps;
	efi_status_t status;

	caps.size = sizeof(caps);
	status = plat->proto->get_capability(plat->proto, &caps);
	if (status != EFI_SUCCESS)
		return -EINVAL;

	if (!caps.tpm_present_flag)
		return -ENODEV;

	priv->pcr_count = 24;
	priv->pcr_select_min = 3;
	priv->version = TPM_V2;

	return 0;
}

static int efi_tpm_open(struct udevice *dev)
{
	return 0;
}

static int efi_tpm_close(struct udevice *dev)
{
	return 0;
}

static int efi_tpm_xfer(struct udevice *dev, const u8 *sendbuf,
			size_t send_size, u8 *recvbuf,
			size_t *recv_len)
{
	struct efi_tpm_plat *plat = dev_get_plat(dev);
	efi_status_t status;

	status = plat->proto->submit_command(plat->proto, send_size,
					     (u8 *)sendbuf, *recv_len,
					     recvbuf);
	switch (status) {
	case EFI_BUFFER_TOO_SMALL:
		debug("%s:response length is bigger than receive buffer\n",
		      __func__);
		return -EIO;
	case EFI_DEVICE_ERROR:
		debug("%s:received error from device on write\n", __func__);
		return -EIO;
	case EFI_INVALID_PARAMETER:
		debug("%s:invalid parameter\n", __func__);
		return -EINVAL;
	case EFI_SUCCESS:
		return 0;
	default:
		debug("%s:received unknown error 0x%lx\n", __func__, status);
		return -EIO;
	}
}

static int efi_tpm_desc(struct udevice *dev, char *buf, int size)
{
	if (size < 9)
		return -ENOSPC;

	return snprintf(buf, size, "UEFI TPM");
}

static const struct tpm_ops efi_tpm_ops = {
	.open		= efi_tpm_open,
	.close		= efi_tpm_close,
	.get_desc	= efi_tpm_desc,
	.xfer		= efi_tpm_xfer,
};

U_BOOT_DRIVER(efi_tpm) = {
	.name	= "efi_tpm",
	.id	= UCLASS_TPM,
	.bind	= efi_tpm_bind,
	.ops	= &efi_tpm_ops,
	.plat_auto = sizeof(struct efi_tpm_plat),
};
