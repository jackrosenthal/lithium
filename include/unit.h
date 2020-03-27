/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef LITHIUM_UNIT_H_
#define LITHIUM_UNIT_H_

/**
 * Lithium Unit
 * ============
 * Core unit testing functionality.
 */

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include "util/reallocating_buffer.h"
#include "macrolib.h"

/**
 * Attribute which should be specified on functions only available
 * during unit tests.
 */
#undef __test_only
#ifdef LITHIUM_TEST_BUILD
#define __test_only
#else
#define __test_only \
	__error_if_used("This function may only be used in unit tests.")
#endif

/**
 * Optional parameters for a test. Default values are always zero,
 * since this is used for static variables.
 */
struct li_unit_test_options {
	/**
	 * True if test failures should be ignored.
	 */
	bool informational;

	/**
	 * True if the test should not be run (consider
	 * ``informational`` as a lighterweight hammer).
	 */
	bool disabled;

	/**
	 * An integer-valued multiple of the default timeout. Disable
	 * timeouts if -1.
	 */
	int timeout_multiplier;
};

/**
 * An entry for a single test.
 */
struct li_unit_test {
	/**
	 * The name of the test (see documentation for
	 * :c:macro:`DEFTEST` for conventions).
	 */
	const char *name;

	/**
	 * Any options associated with this test.
	 */
	struct li_unit_test_options options;

	/**
	 * The function to call to run the test.
	 */
	void (*func)(void);

	/**
	 * Private state used by the test runner.
	 */
	struct {
		enum { _LI_UNIT_NOT_STARTED,
		       _LI_UNIT_RUNNING,
		       _LI_UNIT_PENDING_DEADLINE_EXCEEDED,
		       _LI_UNIT_SUCCEEDED,
		       _LI_UNIT_FAILED,
		       _LI_UNIT_DEADLINE_EXCEEDED,
		} state;
		pid_t pid;
		bool has_deadline;
		struct timespec start_time;
		struct timespec elapsed_time;
		struct timespec deadline;
		int output_pipe[2];
		struct li_reallocating_buffer output;
	} priv;

	/**
	 * The next test to run upon completion.
	 */
	struct li_unit_test *rest;
};

/**
 * The linked list of tests to run.
 */
extern struct li_unit_test *li_unit_test_list;

/**
 * Should be called by the main function for the unit tests.
 *
 * :param argv: The ``argv`` passed to main.
 * :return: 0 on success, 1 if any test failed.
 */
int li_unit_run_tests_main(const char *const *argv);

/**
 * Options struct for :c:func:`li_unit_run_tests`.
 */
struct li_unit_runner_options {
	/**
	 * Integer-valued seconds to time out a single test after. -1
	 * for no timeouts.
	 */
	int default_timeout;

	/**
	 * Number of jobs to run in parallel.
	 */
	unsigned int parallelism;

	/**
	 * How frequently to provide status updates (in seconds).
	 */
	unsigned int status_update_frequency;

	/**
	 * Filter function to determine which tests to run. Takes a
	 * :c:type:`struct li_unit_test *` and a user-defined
	 * data parameter, and should return true if the test should
	 * run, false if not.
	 */
	struct {
		void *data;
		bool (*func)(struct li_unit_test *test, void *data);
	} filter;

	/**
	 * The list of tests to run.
	 */
	struct li_unit_test *test_list;
};

/**
 * Alternative to :c:func:`li_unit_run_tests_main` which takes a
 * options struct instead of command line arguments.
 *
 * :return: 0 if all tests succeded, 1 if any tests failed, or -1 on
 *          failure.
 */
int li_unit_run_tests(struct li_unit_runner_options *options);

/**
 * Run a single test and exit the process with the test's status. This
 * function ignores the informational/disabled status, and does not
 * enforce timeouts.
 */
void __noreturn li_unit_run_test(const struct li_unit_test *test);

/**
 * Macro used to define a test.
 *
 * :param name: The name of the test, as a string literal. This should
 *              follow a naming strategy similar to a domain name (but
 *              reversed): a period-separated list, starting with the
 *              library or program name, then the subsystem name and
 *              any further hierarchical components, and finally the
 *              name of the test.
 * :param options: A :c:type:`struct li_unit_test_options` literal.
 */
#ifdef LITHIUM_TEST_BUILD
#define DEFTEST(name, options) \
	_LI_DEFTEST(name, CONCAT2(li_testfunc_, __LINE__), options)
#else
#define DEFTEST(name, options)                                               \
	static void __maybe_unused __discard CONCAT2(li_discarded_testfunc_, \
						     __LINE__)(void)
#endif

#define _LI_DEFTEST(name, function_id, options) \
	_LI_DEFTEST2(name, function_id, options)

#define _LI_DEFTEST2(NAME, FUNCTION_ID, OPTIONS)            \
	static void FUNCTION_ID(void);                      \
	static __constructor void setup_##FUNCTION_ID(void) \
	{                                                   \
		static struct li_unit_test this_test = {    \
			.name = NAME,                       \
			.func = FUNCTION_ID,                \
			.options = OPTIONS,                 \
		};                                          \
		this_test.rest = li_unit_test_list;         \
		li_unit_test_list = &this_test;             \
	}                                                   \
	static void FUNCTION_ID(void)

void _li_unit_test_assert(bool result, const char *fail_msg);
bool _li_unit_test_expect(bool result, const char *fail_msg);

void _li_unit_test_assert_null(const void *ptr, const char *fail_msg);
bool _li_unit_test_expect_null(const void *ptr, const char *fail_msg);

void _li_unit_test_assert_not_null(const void *ptr, const char *fail_msg);
bool _li_unit_test_expect_not_null(const void *ptr, const char *fail_msg);

void __test_only _li_unit_test_error_assert(bool discarded_expr);
bool __test_only _li_unit_test_error_expect(bool discarded_expr);

#define _LI_UNIT_FORMAT_FAIL_STRING2(macro, file, line, reason, data) \
	file ":" #line ": " #macro " failure, " reason ": " #data

#define _LI_UNIT_FORMAT_FAIL_STRING(macro, file, line, reason, data) \
	_LI_UNIT_FORMAT_FAIL_STRING2(macro, file, line, reason, data)

#ifdef LITHIUM_TEST_BUILD
#define ASSERT(condition)                                               \
	_li_unit_test_assert(                                           \
		condition,                                              \
		_LI_UNIT_FORMAT_FAIL_STRING(ASSERT, __FILE__, __LINE__, \
					    "condition is false", condition))

#define EXPECT(condition)                                               \
	_li_unit_test_expect(                                           \
		condition,                                              \
		_LI_UNIT_FORMAT_FAIL_STRING(EXPECT, __FILE__, __LINE__, \
					    "condition is false", condition))

#define ASSERT_NULL(ptr)                                                     \
	_li_unit_test_assert_null(                                           \
		ptr,                                                         \
		_LI_UNIT_FORMAT_FAIL_STRING(ASSERT_NULL, __FILE__, __LINE__, \
					    "pointer is not NULL", ptr))
#define EXPECT_NULL(ptr)                                                     \
	_li_unit_test_expect_null(                                           \
		ptr,                                                         \
		_LI_UNIT_FORMAT_FAIL_STRING(EXPECT_NULL, __FILE__, __LINE__, \
					    "pointer is not NULL", ptr))

#define ASSERT_NOT_NULL(ptr)                                           \
	_li_unit_test_assert_not_null(                                 \
		ptr,                                                   \
		_LI_UNIT_FORMAT_FAIL_STRING(ASSERT_NOT_NULL, __FILE__, \
					    __LINE__, "pointer is NULL", ptr))
#define EXPECT_NOT_NULL(ptr)                                           \
	_li_unit_test_expect_not_null(                                 \
		ptr,                                                   \
		_LI_UNIT_FORMAT_FAIL_STRING(EXPECT_NOT_NULL, __FILE__, \
					    __LINE__, "pointer is NULL", ptr))
#else
#define ASSERT(expr) _li_unit_test_error_assert(expr)
#define EXPECT(expr) _li_unit_test_error_expect(expr)
#define ASSERT_NULL(expr) _li_unit_test_error_assert(expr)
#define EXPECT_NULL(expr) _li_unit_test_error_expect(expr)
#define ASSERT_NOT_NULL(expr) _li_unit_test_error_assert(expr)
#define EXPECT_NOT_NULL(expr) _li_unit_test_error_expect(expr)
#endif

#endif /* LITHIUM_UNIT_H_ */
