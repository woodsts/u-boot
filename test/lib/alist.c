// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <alist.h>
#include <string.h>
#include <test/lib.h>
#include <test/test.h>
#include <test/ut.h>

struct my_struct {
	uint val;
	uint other_val;
};

enum {
	obj_size	= sizeof(struct my_struct),
};

/* Test alist_init() */
static int lib_test_alist_init(struct unit_test_state *uts)
{
	struct alist lst;
	ulong start;

	start = ut_check_free();

	/* with a size of 0, the fields should be inited, with no memory used */
	memset(&lst, '\xff', sizeof(lst));
	ut_assert(alist_init_struct(&lst, struct my_struct));
	ut_asserteq_ptr(NULL, lst.data);
	ut_asserteq(0, lst.count);
	ut_asserteq(0, lst.alloc);
	ut_assertok(ut_check_delta(start));
	alist_uninit(&lst);
	ut_asserteq_ptr(NULL, lst.data);
	ut_asserteq(0, lst.count);
	ut_asserteq(0, lst.alloc);

	/* use an impossible size */
	ut_asserteq(false, alist_init(&lst, obj_size,
				      CONFIG_SYS_MALLOC_LEN));
	ut_assertnull(lst.data);
	ut_asserteq(0, lst.count);
	ut_asserteq(0, lst.alloc);

	/* use a small size */
	ut_assert(alist_init(&lst, obj_size, 4));
	ut_assertnonnull(lst.data);
	ut_asserteq(0, lst.count);
	ut_asserteq(4, lst.alloc);

	/* free it */
	alist_uninit(&lst);
	ut_asserteq_ptr(NULL, lst.data);
	ut_asserteq(0, lst.count);
	ut_asserteq(0, lst.alloc);
	ut_assertok(ut_check_delta(start));

	/* Check for memory leaks */
	ut_assertok(ut_check_delta(start));

	return 0;
}
LIB_TEST(lib_test_alist_init, 0);

/* Test alist_get() and alist_getd() */
static int lib_test_alist_get(struct unit_test_state *uts)
{
	struct alist lst;
	ulong start;
	void *ptr;

	start = ut_check_free();

	ut_assert(alist_init(&lst, obj_size, 3));
	ut_asserteq(0, lst.count);
	ut_asserteq(3, lst.alloc);

	ut_assertnull(alist_get_ptr(&lst, 2));
	ut_assertnull(alist_get_ptr(&lst, 3));

	ptr = alist_ensure_ptr(&lst, 1);
	ut_assertnonnull(ptr);
	ut_asserteq(2, lst.count);
	ptr = alist_ensure_ptr(&lst, 2);
	ut_asserteq(3, lst.count);
	ut_assertnonnull(ptr);

	ptr = alist_ensure_ptr(&lst, 3);
	ut_assertnonnull(ptr);
	ut_asserteq(4, lst.count);
	ut_asserteq(6, lst.alloc);

	ut_assertnull(alist_get_ptr(&lst, 4));

	alist_uninit(&lst);

	/* Check for memory leaks */
	ut_assertok(ut_check_delta(start));

	return 0;
}
LIB_TEST(lib_test_alist_get, 0);

/* Test alist_has() */
static int lib_test_alist_has(struct unit_test_state *uts)
{
	struct alist lst;
	ulong start;
	void *ptr;

	start = ut_check_free();

	ut_assert(alist_init(&lst, obj_size, 3));

	ut_assert(!alist_has(&lst, 0));
	ut_assert(!alist_has(&lst, 1));
	ut_assert(!alist_has(&lst, 2));
	ut_assert(!alist_has(&lst, 3));

	/* create a new one to force expansion */
	ptr = alist_ensure_ptr(&lst, 4);
	ut_assertnonnull(ptr);

	ut_assert(alist_has(&lst, 0));
	ut_assert(alist_has(&lst, 1));
	ut_assert(alist_has(&lst, 2));
	ut_assert(alist_has(&lst, 3));
	ut_assert(alist_has(&lst, 4));
	ut_assert(!alist_has(&lst, 5));

	alist_uninit(&lst);

	/* Check for memory leaks */
	ut_assertok(ut_check_delta(start));

	return 0;
}
LIB_TEST(lib_test_alist_has, 0);

/* Test alist_ensure() */
static int lib_test_alist_ensure(struct unit_test_state *uts)
{
	struct my_struct *ptr3, *ptr4;
	struct alist lst;
	ulong start;

	start = ut_check_free();

	ut_assert(alist_init_struct(&lst, struct my_struct));
	ut_asserteq(obj_size, lst.obj_size);
	ut_asserteq(0, lst.count);
	ut_asserteq(0, lst.alloc);
	ptr3 = alist_ensure_ptr(&lst, 3);
	ut_asserteq(4, lst.count);
	ut_asserteq(4, lst.alloc);
	ut_assertnonnull(ptr3);
	ptr3->val = 3;

	ptr4 = alist_ensure_ptr(&lst, 4);
	ut_asserteq(8, lst.alloc);
	ut_asserteq(5, lst.count);
	ut_assertnonnull(ptr4);
	ptr4->val = 4;
	ut_asserteq(4, alist_get(&lst, 4, struct my_struct)->val);

	ut_asserteq_ptr(ptr4, alist_ensure(&lst, 4, struct my_struct));

	alist_ensure(&lst, 4, struct my_struct)->val = 44;
	ut_asserteq(44, alist_get(&lst, 4, struct my_struct)->val);
	ut_asserteq(3, alist_get(&lst, 3, struct my_struct)->val);
	ut_assertnull(alist_get(&lst, 7, struct my_struct));
	ut_asserteq(8, lst.alloc);
	ut_asserteq(5, lst.count);

	/* add some more, checking handling of malloc() failure */
	malloc_enable_testing(0);
	ut_assertnonnull(alist_ensure(&lst, 7, struct my_struct));
	ut_assertnull(alist_ensure(&lst, 8, struct my_struct));
	malloc_disable_testing();

	lst.flags &= ~ALISTF_FAIL;
	ut_assertnonnull(alist_ensure(&lst, 8, struct my_struct));
	ut_asserteq(16, lst.alloc);
	ut_asserteq(9, lst.count);

	alist_uninit(&lst);

	/* Check for memory leaks */
	ut_assertok(ut_check_delta(start));

	return 0;
}
LIB_TEST(lib_test_alist_ensure, 0);
