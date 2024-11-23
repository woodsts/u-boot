// SPDX-License-Identifier: GPL-2.0+
/*
 * Aurora Innovation, Inc. Copyright 2022.
 *
 */

#include <config.h>
#include <dm.h>
#include <efi.h>
#include <efi_api.h>
#include <malloc.h>
#include <net.h>

struct efi_net_priv {
	void *buffer;
};

static int efi_net_bind(struct udevice *dev)
{
	struct efi_net_plat *plat = dev_get_plat(dev);
	struct efi_net_priv *priv = dev_get_priv(dev);
	efi_status_t status;

	priv->buffer = malloc(PKTSIZE);

	status = plat->snp->start(plat->snp);
	if (status != EFI_SUCCESS)
		return -ENOENT;

	return 0;
}

static int efi_net_init(struct udevice *dev)
{
	struct efi_net_plat *plat = dev_get_plat(dev);
	efi_status_t status;

	status = plat->snp->initialize(plat->snp, 0, 0);
	if (status != EFI_SUCCESS)
		return -ENOENT;

	return 0;
}

static void efi_net_halt(struct udevice *dev)
{
	struct efi_net_plat *plat = dev_get_plat(dev);

	plat->snp->shutdown(plat->snp);
	plat->snp->stop(plat->snp);
}

static int efi_net_send(struct udevice *dev, void *packet, int length)
{
	struct efi_net_plat *plat = dev_get_plat(dev);
	efi_status_t status;

	status = plat->snp->transmit(plat->snp, 0, length, packet, NULL, NULL,
				     NULL);
	if (status != EFI_SUCCESS)
		return -EINVAL;

	return 0;
}

static int efi_net_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct efi_net_plat *plat = dev_get_plat(dev);
	struct efi_net_priv *priv = dev_get_priv(dev);
	efi_status_t status;
	size_t size = PKTSIZE;

	status = plat->snp->receive(plat->snp, 0, &size, priv->buffer, NULL,
				    NULL, NULL);
	if (status == EFI_NOT_READY)
		return -EAGAIN;
	else if (status != EFI_SUCCESS)
		return -EINVAL;

	*packetp = priv->buffer;

	return 0;
}

static int efi_net_read_hwaddr(struct udevice *dev)
{
	struct efi_net_plat *plat = dev_get_plat(dev);

	memcpy(plat->eth_pdata.enetaddr,
	       (void *)&plat->snp->mode->permanent_address, ARP_HLEN);

	return 0;
}

static const struct eth_ops efi_net_ops = {
	.start	= efi_net_init,
	.stop	= efi_net_halt,
	.send	= efi_net_send,
	.recv	= efi_net_recv,
	.read_rom_hwaddr = efi_net_read_hwaddr,
};

U_BOOT_DRIVER(efi_net) = {
	.name	= "efi_net",
	.id	= UCLASS_ETH,
	.bind	= efi_net_bind,
	.ops	= &efi_net_ops,
	.plat_auto = sizeof(struct efi_net_plat),
	.priv_auto = sizeof(struct efi_net_priv),
};
