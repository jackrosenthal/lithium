/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdbool.h>
#include <time.h>

#include "constants.h"
#include "unit.h"
#include "util/timespec.h"

void li_util_timespec_subtract(const struct timespec *a,
			       const struct timespec *b,
			       struct timespec *result)
{
	result->tv_sec = a->tv_sec - b->tv_sec;

	if (a->tv_nsec < b->tv_nsec) {
		result->tv_sec -= 1;
		result->tv_nsec = (NSEC_PER_SEC - b->tv_nsec) + a->tv_nsec;
	} else {
		result->tv_nsec = a->tv_nsec - b->tv_nsec;
	}
}

DEFTEST("lithium.util.timespec.subtract", {})
{
	struct timespec t1 = { 1000, 123456 };
	struct timespec t2 = { 1000, 456 };
	struct timespec t3 = { 1002, 456 };
	struct timespec out;

	li_util_timespec_subtract(&t1, &t2, &out);
	EXPECT(out.tv_sec == 0);
	EXPECT(out.tv_nsec == 123000);

	li_util_timespec_subtract(&t3, &t1, &out);
	EXPECT(out.tv_sec == 1);
	EXPECT(out.tv_nsec == 999877000);

	li_util_timespec_subtract(&t3, &t3, &out);
	EXPECT(out.tv_sec == 0);
	EXPECT(out.tv_nsec == 0);
}

bool li_util_timespec_lt(const struct timespec *a, const struct timespec *b)
{
	if (a->tv_sec < b->tv_sec)
		return true;
	if (a->tv_sec == b->tv_sec)
		return a->tv_nsec < b->tv_nsec;
	return false;
}

DEFTEST("lithium.util.timespec.lt", {})
{
	struct timespec t1 = { 1000, 123456 };
	struct timespec t2 = { 1000, 456 };
	struct timespec t3 = { 1002, 456 };

	EXPECT(li_util_timespec_lt(&t1, &t2) == false);
	EXPECT(li_util_timespec_lt(&t2, &t1) == true);
	EXPECT(li_util_timespec_lt(&t3, &t1) == false);
	EXPECT(li_util_timespec_lt(&t3, &t3) == false);
	EXPECT(li_util_timespec_lt(&t1, &t3) == true);
}
