// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <efi_log.h>
#include <mapmem.h>
#include <test/lib.h>
#include <test/test.h>
#include <test/ut.h>

/* basic test of logging */
static int lib_test_efi_log_base(struct unit_test_state *uts)
{
	void **buf = map_sysmem(0x1000, 0);
	u64 *addr = map_sysmem(0x1010, 0);
	int ofs1, ofs2;

	ut_assertok(efi_log_reset());

	ofs1 = efi_logs_testing(EFI_LOG_TEST0, 123, &buf[0], &addr[0]);

	ofs2 = efi_logs_testing(EFI_LOG_TEST1, 456, &buf[1], &addr[1]);

	/* simulate an EFI call setting the return values */
	addr[0] = 0x100;
	buf[0] = map_sysmem(0x1100, 0);
	addr[1] = 0x200;
	buf[1] = map_sysmem(0x1200, 0);

	ut_assertok(efi_loge_testing(ofs2, EFI_LOAD_ERROR));
	ut_assertok(efi_loge_testing(ofs1, EFI_SUCCESS));

	ut_assertok(efi_log_show());
	ut_assert_nextline("EFI log (size 98)");
	ut_assert_nextline(
		"  0      testing test0 int 7b/123 buf 1000 mem 1010 *buf 1100 *mem 100 ret OK");
	ut_assert_nextline(
		"  1      testing test1 int 1c8/456 buf 1008 mem 1018 *buf 1200 *mem 200 ret load");
	ut_assert_nextline("2 records");
	ut_assert_console_end();

	unmap_sysmem(buf);
	unmap_sysmem(addr);

	return 0;
}
LIB_TEST(lib_test_efi_log_base, UTF_CONSOLE);

/* test the memory-function logging */
static int lib_test_efi_log_mem(struct unit_test_state *uts)
{
	void **buf = map_sysmem(0x1000, 0);
	u64 *addr = map_sysmem(0x1010, 0);
	int ofs1, ofs2;

	ut_assertok(efi_log_reset());

	ofs1 = efi_logs_allocate_pool(EFI_BOOT_SERVICES_DATA, 100, buf);
	ofs2 = efi_logs_allocate_pages(EFI_ALLOCATE_ANY_PAGES,
				       EFI_BOOT_SERVICES_CODE, 10, addr);
	ut_assertok(efi_loge_allocate_pages(ofs2, EFI_LOAD_ERROR));
	ut_assertok(efi_loge_allocate_pool(ofs1, 0));

	ofs1 = efi_logs_free_pool(*buf);
	ut_assertok(efi_loge_free_pool(ofs1, EFI_INVALID_PARAMETER));

	ofs2 = efi_logs_free_pages(*addr, 0);
	ut_assertok(efi_loge_free_pool(ofs2, 0));

	ut_assertok(efi_log_show());

	ut_assert_nextline("EFI log (size c0)");

	/*
	 * We end up with internal sandbox-addresses here since EFI_LOADER
	 * doesn't handle map_sysmem() correctly. So for now, only part of the
	 * string is matched.
	 */
	ut_assert_nextlinen("  0   alloc_pool bt-data size 64/100 buf 10002000 *buf");
	ut_assert_nextlinen("  1  alloc_pages any-pages bt-code pgs a/10 mem 10002010 *mem");
	ut_assert_nextlinen("  2    free_pool buf");
	ut_assert_nextlinen("  3   free_pages mem");

	ut_assert_nextline("4 records");

	unmap_sysmem(buf);
	unmap_sysmem(addr);

	return 0;
}
LIB_TEST(lib_test_efi_log_mem, UTF_CONSOLE);
