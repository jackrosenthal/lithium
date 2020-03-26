/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "unit.h"
#include "mock.h"

static LI_MOCKABLE_STORAGE_T(int) e1;

static int get_e1(void)
{
	return LI_MOCKABLE(e1, 12);
}

DEFTEST("lithium.mock.integer", {})
{
	EXPECT(get_e1() == 12);

	LI_SETUP_MOCK(e1, -15);
	EXPECT(get_e1() == -15);

	LI_SETUP_MOCK(e1, 0);
	EXPECT(get_e1() == 0);
}

static int mockable_function(void)
{
	return 42;
}

static LI_MOCKABLE_STORAGE_T(&mockable_function) e2;

static int get_value_from_mockable_function(void)
{
	return LI_MOCKABLE(e2, mockable_function)();
}

static int mocked_version(void)
{
	return 44;
}

DEFTEST("lithium.mock.function", {})
{
	EXPECT(get_value_from_mockable_function() == 42);

	LI_SETUP_MOCK(e2, mocked_version);
	EXPECT(get_value_from_mockable_function() == 44);
}

struct test_mock_struct {
	int a;
	int b;
	int c;
};

static struct test_mock_struct original_struct = { 1, 2, 3 };

static LI_MOCKABLE_STORAGE_T(struct test_mock_struct *) e3;

static struct test_mock_struct *get_test_struct(void)
{
	return LI_MOCKABLE(e3, &original_struct);
}

DEFTEST("lithium.mock.struct", {})
{
	EXPECT(get_test_struct() == &original_struct);

	struct test_mock_struct mocked_struct = { 4, 5, 6 };
	LI_SETUP_MOCK(e3, &mocked_struct);

	EXPECT(get_test_struct() == &mocked_struct);
}

static LI_MOCKABLE_STORAGE_T(const char *) e4;

static const char *get_original_test_string(void)
{
	return "Original String";
}

static const char *get_test_string(void)
{
	return LI_MOCKABLE(e4, get_original_test_string());
}

DEFTEST("lithium.mock.function_return_value", {})
{
	EXPECT(!strcmp(get_test_string(), "Original String"));

	LI_SETUP_MOCK(e4, "Mocked String!");
	EXPECT(!strcmp(get_test_string(), "Mocked String!"));
}
