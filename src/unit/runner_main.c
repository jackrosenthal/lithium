/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cmdline.h"
#include "unit.h"

static int run_single_test_by_name(const char *name)
{
	for (struct li_unit_test *test = li_unit_test_list; test;
	     test = test->rest) {
		if (!strcmp(test->name, name))
			li_unit_run_test(test);
	}

	fprintf(stderr, "No test named %s!\n", name);
	return 1;
}

int li_unit_run_tests_main(const char *const *argv)
{
	bool list_tests = false;
	struct li_unit_runner_options options = { 0 };
	const char *filterexpr = NULL;
	const char *single = NULL;

	struct li_cmdline_option cmdline_opts[] = {
		{
			.longopt = "list-tests",
			.help = "Lists tests to be run and exit.",
			.action = {
				.type = LI_CMDLINE_STORE_TRUE,
				.dest = &list_tests,
			},
		},
		{
			.shortopt = 's',
			.longopt = "single",
			.help = "Run a single test by this name, with no "
			"timeouts, and exit.",
			.action = {
				.type = LI_CMDLINE_STRING,
				.dest = &single,
			},
		},
		{
			.shortopt = 't',
			.longopt = "timeout",
			.help = "Integer value in seconds for default "
			"timeout. -1 for no timeouts.",
			.action = {
				.type = LI_CMDLINE_SCANF,
				.format = "%d",
				.dest = &options.default_timeout,
			},
		},
		{
			.shortopt = 'j',
			.longopt = "jobs",
			.help = "Maximum number of tests to run in parallel.",
			.action = {
				.type = LI_CMDLINE_SCANF,
				.format = "%u",
				.dest = &options.parallelism,
			},
		},
		{
			.shortopt = 'f',
			.longopt = "filter",
			.help = "An expression to filter which tests to run "
			"(see below).",
			.action = {
				.type = LI_CMDLINE_STRING,
				.dest = &filterexpr,
			},
		},
		{
			.shortopt = 'u',
			.longopt = "status",
			.help = "How frequently to print status updates, in "
			"seconds.",
			.action = {
				.type = LI_CMDLINE_SCANF,
				.format = "%u",
				.dest = &options.status_update_frequency,
			},
		},
		{
			.shortopt = 'h',
			.longopt = "help",
			.action.type = LI_CMDLINE_HELP,
		},
		{ 0 },
	};

	struct li_cmdline spec = {
		.title = "Lithium Test Runner",
		.options = cmdline_opts,
	};

	switch (li_cmdline_parse(&spec, argv, NULL)) {
	case LI_CMDLINE_EXIT_SUCCESS:
		return 0;
	case LI_CMDLINE_CONTINUE:
		if (single)
			return run_single_test_by_name(single);
		return li_unit_run_tests(&options) != 0;
	default:
		return 1;
	}
}
