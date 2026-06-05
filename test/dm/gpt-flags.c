// SPDX-License-Identifier: GPL-2.0+
/*
 * Tests for ChromiumOS-style GPT PTE attributes.
 *
 * Copyright 2026 Ford Motor Company
 * Written by Denis Mukhin <dmukhin@ford.com>
 */

#include <dm.h>
#include <memalign.h>
#include <mmc.h>
#include <part.h>
#include <part_efi.h>
#include <dm/test.h>
#include <test/ut.h>
#include <u-boot/crc.h>

#define TEST_LBA_COUNT  48
#define TEST_PARTNUM    1

int is_gpt_valid(struct blk_desc *desc, u64 lba, gpt_header *pgpt_head,
                 gpt_entry **pgpt_pte);
int is_pte_valid(gpt_entry * pte);

static int mock_blk_desc(struct unit_test_state *uts, struct blk_desc **desc)
{
	struct udevice *dev;
	char str_disk_guid[UUID_STR_LEN + 1];
	struct disk_partition parts[2] = {
		{
			.start = TEST_LBA_COUNT,
			.size = 1,
			.name = "test1",
		},
		{
			.start = 49,
			.size = 1,
			.name = "test2",
		},
	};

	ut_assertok(uclass_get_device_by_name(UCLASS_BLK, "mmc1.blk", &dev));
	*desc = (struct blk_desc *)dev_get_uclass_plat(dev);

	if (CONFIG_IS_ENABLED(RANDOM_UUID)) {
		gen_rand_uuid_str(parts[0].uuid, UUID_STR_FORMAT_STD);
		gen_rand_uuid_str(parts[1].uuid, UUID_STR_FORMAT_STD);
		gen_rand_uuid_str(str_disk_guid, UUID_STR_FORMAT_STD);
	}
	ut_assertok(gpt_restore(*desc, str_disk_guid, parts,
	                        ARRAY_SIZE(parts)));

	return 0;
}

static int test_gpt_flags_default(struct unit_test_state *uts)
{
	struct blk_desc *desc;
	u16 flags;

	ut_assertok(mock_blk_desc(uts, &desc));
	ut_assertok(read_disk_flags(desc, TEST_PARTNUM, &flags));
	ut_asserteq(0, flags);

	return 0;
}
DM_TEST(test_gpt_flags_default, UTF_SCAN_FDT);

static int test_gpt_flags_some(struct unit_test_state *uts)
{
	struct blk_desc *desc;
	u16 flags_in, flags_out;

	ut_assertok(mock_blk_desc(uts, &desc));

	flags_in = 0x0111;  /* priority=1, tries=1, successful=1 */
	ut_assertok(write_disk_flags(desc, TEST_PARTNUM, flags_in));
	ut_assertok(read_disk_flags(desc, TEST_PARTNUM, &flags_out));
	ut_asserteq(flags_in, flags_out);

	return 0;
}
DM_TEST(test_gpt_flags_some, UTF_SCAN_FDT);

static int test_gpt_flags_update(struct unit_test_state *uts)
{
	struct blk_desc *desc;
	u16 flags;

	ut_assertok(mock_blk_desc(uts, &desc));
	ut_assertok(write_disk_flags(desc, TEST_PARTNUM, 0x00FF));

	/* other partitions must remain zero */
	ut_assertok(read_disk_flags(desc, TEST_PARTNUM + 1, &flags));
	ut_asserteq(0, flags);

	return 0;
}
DM_TEST(test_gpt_flags_update, UTF_SCAN_FDT);

static int test_disk_flags_invalid_partnum(struct unit_test_state *uts)
{
	struct blk_desc *desc;
	u16 flags;

	ut_assertok(mock_blk_desc(uts, &desc));
	ut_asserteq(-EINVAL, read_disk_flags(desc, 0, &flags));
	ut_asserteq(-EINVAL, write_disk_flags(desc, 0, 0));
	ut_asserteq(-EINVAL, write_disk_flags(desc, 255, 0));

	return 0;
}
DM_TEST(test_disk_flags_invalid_partnum, UTF_SCAN_FDT);

static int test_gpt_flags_headers(struct unit_test_state *uts)
{
	struct blk_desc *desc;
	u16 flags_in = 0x0111;

	ut_assertok(mock_blk_desc(uts, &desc));
	ut_assertok(write_disk_flags(desc, TEST_PARTNUM, flags_in));

	{
		ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_h1, 1, desc->blksz);
		ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_h2, 1, desc->blksz);
		gpt_entry *gpt_primary = NULL, *gpt_backup = NULL;
		u64 primary_attrs, backup_attrs;

		ut_asserteq(1, is_gpt_valid(desc, GPT_PRIMARY_PARTITION_TABLE_LBA, gpt_h1,
		                            &gpt_primary));
		ut_asserteq(1, is_pte_valid(&gpt_primary[TEST_PARTNUM - 1]));

		ut_asserteq(1, is_gpt_valid(desc, desc->lba - 1, gpt_h2, &gpt_backup));
		ut_asserteq(1, is_pte_valid(&gpt_backup[TEST_PARTNUM - 1]));

		primary_attrs = le64_to_cpu(gpt_primary[TEST_PARTNUM - 1].attributes.raw);
		ut_asserteq(flags_in, (primary_attrs >> 48) & 0xFFFFULL);

		backup_attrs  = le64_to_cpu(gpt_backup[TEST_PARTNUM - 1].attributes.raw);
		ut_asserteq(flags_in, (backup_attrs >> 48) & 0xFFFFULL);

		free(gpt_primary);
		free(gpt_backup);
	}

	return 0;
}
DM_TEST(test_gpt_flags_headers, UTF_SCAN_FDT);
