/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef LITHIUM_MOCK_H_
#define LITHIUM_MOCK_H_

#include <stdbool.h>

#include "macrolib.h"
#include "unit.h"

/**
 * Lithium Mock
 * ============
 */

#ifdef LITHIUM_TEST_BUILD
#define LI_MOCKABLE_STORAGE_T(data_type) \
	struct {                         \
		bool mocked;             \
		typeof(data_type) value; \
	}

#define LI_SETUP_MOCK(expr_id, data)      \
	do {                              \
		(expr_id).mocked = true;  \
		(expr_id).value = (data); \
	} while (0)

#define LI_MOCKABLE(expr_id, default_value) \
	((expr_id).mocked ? (expr_id).value : (default_value))

#else
/* Empty macros for non-test builds */

#define LI_MOCKABLE_STORAGE_T(data_type) \
	__maybe_unused struct {          \
	}

#define LI_SETUP_MOCK(expr_id, data)                           \
	do {                                                   \
		(void)(data);                                  \
		extern void __error_if_used(                   \
			"LI_SETUP_MOCK cannot be used unless " \
			"LITHIUM_TEST_BUILD is defined.")      \
			_li_setup_mock_fail(void);             \
		_li_setup_mock_fail();                         \
	} while (0)

#define LI_MOCKABLE(expr_id, default_value) (default_value)

#endif /* LITHIUM_TEST_BUILD */

#endif /* LITHIUM_MOCK_H_ */
