/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef LITHIUM_UTIL_TIMESPEC_H_
#define LITHIUM_UTIL_TIMESPEC_H_

#include <stdbool.h>

struct timespec;

void li_util_timespec_subtract(const struct timespec *a,
			       const struct timespec *b,
			       struct timespec *result);

bool li_util_timespec_lt(const struct timespec *a, const struct timespec *b);

#endif /* LITHIUM_UTIL_TIMESPEC_H_ */
