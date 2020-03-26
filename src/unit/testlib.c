/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "macrolib.h"
#include "unit.h"

static size_t successful_assertions;
static size_t failed_assertions;

static void __noreturn handle_test_exit(bool premature)
{
	if (premature)
		printf("Test ended prematurely due to an assertion failure!\n");
	else if (failed_assertions)
		printf("Test ended, but failed due to expectation failures!\n");
	else
		printf("Test succeeded!\n");

	printf("%zu successful assertions, %zu failed assertions!\n",
	       successful_assertions, failed_assertions);

	exit(failed_assertions != 0);
	__builtin_unreachable();
}

void _li_unit_test_assert(bool result, const char *fail_msg)
{
	if (result) {
		successful_assertions += 1;
	} else {
		failed_assertions += 1;
		printf("%s\n", fail_msg);
		handle_test_exit(true);
	}
}

bool _li_unit_test_expect(bool result, const char *fail_msg)
{
	if (result) {
		successful_assertions += 1;
	} else {
		failed_assertions += 1;
		printf("%s\n", fail_msg);
	}

	return result;
}

void _li_unit_test_assert_null(const void *ptr, const char *fail_msg)
{
	_li_unit_test_assert(!ptr, fail_msg);
}

bool _li_unit_test_expect_null(const void *ptr, const char *fail_msg)
{
	return _li_unit_test_expect(!ptr, fail_msg);
}

void _li_unit_test_assert_not_null(const void *ptr, const char *fail_msg)
{
	_li_unit_test_assert(ptr, fail_msg);
}

bool _li_unit_test_expect_not_null(const void *ptr, const char *fail_msg)
{
	return _li_unit_test_expect(ptr, fail_msg);
}

void __noreturn li_unit_run_test(const struct li_unit_test *test)
{
	printf("Running test %s...\n", test->name);

	if (test->options.informational)
		printf("NOTICE: Test is informational.\n");
	if (test->options.disabled)
		printf("WARNING: Test is disabled. Running anyway.\n");

	test->func();
	handle_test_exit(false);
}
