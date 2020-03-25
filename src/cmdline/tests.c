/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "cmdline.h"
#include "unit.h"

DEFTEST("lithium.cmdline.smoke", {})
{
	const char *const argv[] = { "foo", NULL };

	struct li_cmdline spec;
	EXPECT(li_cmdline_parse(&spec, argv, NULL) == LI_CMDLINE_CONTINUE);
}

DEFTEST("lithium.cmdline.shortopts.separated", {})
{
	const char *const argv[] = {
		"progname", "-t", "15", "-T", "-q", "aaa", NULL,
	};

	int t = -1;
	bool bigt = false;
	const char *q = NULL;

	struct li_cmdline_option options[] = {
		{
			.shortopt = 't',
			.action.type = LI_CMDLINE_SCANF,
			.action.format = "%d",
			.action.dest = &t,
		},
		{
			.shortopt = 'T',
			.action.type = LI_CMDLINE_STORE_TRUE,
			.action.dest = &bigt,
		},
		{
			.shortopt = 'q',
			.action.type = LI_CMDLINE_STRING,
			.action.dest = &q,
		},
		{
			.shortopt = 'h',
			.action.type = LI_CMDLINE_HELP,
		},
		{ 0 },
	};

	struct li_cmdline spec = { .options = options };
	ASSERT(li_cmdline_parse(&spec, argv, NULL) == LI_CMDLINE_CONTINUE);

	EXPECT(t == 15);
	EXPECT(bigt == true);
	EXPECT(!strcmp(q, "aaa"));
}

DEFTEST("lithium.cmdline.shortopts.joined", {})
{
	const char *const argv[] = {
		"progname", "-t15", "-T", "-qaaa", NULL,
	};

	int t = -1;
	bool bigt = true;
	const char *q = NULL;

	struct li_cmdline_option options[] = {
		{
			.shortopt = 't',
			.action.type = LI_CMDLINE_SCANF,
			.action.format = "%d",
			.action.dest = &t,
		},
		{
			.shortopt = 'T',
			.action.type = LI_CMDLINE_STORE_FALSE,
			.action.dest = &bigt,
		},
		{
			.shortopt = 'q',
			.action.type = LI_CMDLINE_STRING,
			.action.dest = &q,
		},
		{
			.shortopt = 'h',
			.action.type = LI_CMDLINE_HELP,
		},
		{ 0 },
	};

	struct li_cmdline spec = { .options = options };
	ASSERT(li_cmdline_parse(&spec, argv, NULL) == LI_CMDLINE_CONTINUE);

	EXPECT(t == 15);
	EXPECT(bigt == false);
	EXPECT(!strcmp(q, "aaa"));
}
