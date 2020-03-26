/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "unit.h"

static void wait_2_seconds(void)
{
	sleep(2);
}

DEFTEST("lithium.unit.runner.timeout", {})
{
	struct li_unit_test timeout_test = {
		.name = "should_timeout",
		.func = wait_2_seconds,
		.options = {
			.timeout_multiplier = 1,
		},
	};

	struct li_unit_runner_options options = {
		.default_timeout = 1,
		.parallelism = 1,
		.test_list = &timeout_test,
	};

	EXPECT(li_unit_run_tests(&options) == 1);
}

static void test_failure(void)
{
	EXPECT(false);
}

DEFTEST("lithium.unit.runner.failure", {})
{
	struct li_unit_test failing_test = {
		.name = "should_fail",
		.func = test_failure,
		.options = {
			.timeout_multiplier = 1,
		},
	};

	struct li_unit_runner_options options = {
		.default_timeout = 1,
		.parallelism = 1,
		.test_list = &failing_test,
	};

	EXPECT(li_unit_run_tests(&options) == 1);
}

DEFTEST("lithium.unit.runner.spam_output", {})
{
	for (size_t i = 0; i < PIPE_BUF; i++) {
		for (char c = 'a'; c <= 'z'; c++) {
			putchar(c);
		}
	}
}
