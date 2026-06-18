// SPDX-License-Identifier: GPL-2.0+

#define pr_fmt(fmt)		"bootcount-gpt: " fmt
#define LOG_CATEGORY	UCLASS_BOOTCOUNT

#include <blk.h>
#include <bootcount.h>
#include <dm.h>
#include <linux/printk.h>
#include <part.h>

struct bootcount_gpt_priv {
	struct blk_desc *blk_desc;
	const char *partition_name;
	int uclass_id;
	int devnum;
	int partnum;
};

static int bootcount_gpt_find(struct udevice *dev)
{
	struct bootcount_gpt_priv *priv = dev_get_priv(dev);
	struct udevice *bdev;
	struct disk_partition info;
	int partnum, rc;

	rc = blk_get_device(priv->uclass_id, priv->devnum, &bdev);
	if (rc < 0)
		return -ENODEV;

	partnum = part_get_info_by_name(dev_get_uclass_plat(bdev),
	                                priv->partition_name, &info);
	if (partnum < 0)
		return -ENODEV;

	if (info.bootable & PART_BOOTABLE) {
		pr_warn("partition %s: legacy boot flag enabled, no GPT accounting\n",
		        priv->partition_name);
		return -EPERM;
	}

	priv->blk_desc = dev_get_uclass_plat(bdev);
	priv->partnum = partnum;

	return 0;
}

static int bootcount_gpt_get(struct udevice *dev, u32 *val)
{
	struct bootcount_gpt_priv *priv = dev_get_priv(dev);
	int rc;

	if (!priv->blk_desc) {
		rc = bootcount_gpt_find(dev);
		if (rc < 0)
			return rc;
	}

	rc = read_disk_flags(priv->blk_desc, priv->partnum, (u16 *)val);
	if (rc < 0) {
		pr_err("partnum %d: failed to read GPT flags: %d\n",
		       priv->partnum, rc);
		return rc;
	}

	pr_debug("partnum %d: read  GPT flags 0x%04x\n", priv->partnum, *val);

	return 0;
}

static int bootcount_gpt_set(struct udevice *dev, const u32 val)
{
	struct bootcount_gpt_priv *priv = dev_get_priv(dev);
	int rc;

	if (!priv->blk_desc) {
		rc = bootcount_gpt_find(dev);
		if (rc < 0)
			return rc;
	}

	pr_debug("partnum %d: write GPT flags 0x%04x\n", priv->partnum, val);

	rc = write_disk_flags(priv->blk_desc, priv->partnum, (u16)val);
	if (rc < 0) {
		pr_err("partnum %d: failed to write GPT flags: %d\n",
		       priv->partnum, rc);
		return rc;
	}

	return 0;
}

static int bootcount_gpt_probe(struct udevice *dev)
{
	struct bootcount_gpt_priv *priv = dev_get_priv(dev);
	const char *str;
	int uclass_id, devnum;

	str = dev_read_string(dev, "device-type");
	if (!str)
		return -ENOSYS;

	uclass_id = uclass_get_by_name(str);
	if (uclass_id == UCLASS_INVALID)
		return -ENOSYS;

	devnum = dev_read_s32_default(dev, "devnum", 0);
	if (devnum < 0)
		return -ENOSYS;

	str = dev_read_string(dev, "partition-name");
	if (!str)
		return -ENOSYS;

	priv->uclass_id = uclass_id;
	priv->devnum = devnum;
	priv->partition_name = str;

	return 0;
}

static const struct bootcount_ops bootcount_gpt_ops = {
	.get = bootcount_gpt_get,
	.set = bootcount_gpt_set,
};

static const struct udevice_id bootcount_gpt_ids[] = {
	{ .compatible = "u-boot,bootcount-gpt" },
	{ }
};

U_BOOT_DRIVER(bootcount_gpt) = {
	.name		= "bootcount-gpt",
	.id		= UCLASS_BOOTCOUNT,
	.priv_auto	= sizeof(struct bootcount_gpt_priv),
	.probe		= bootcount_gpt_probe,
	.of_match	= bootcount_gpt_ids,
	.ops		= &bootcount_gpt_ops,
};
