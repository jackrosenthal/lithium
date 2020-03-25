/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef LITHIUM_MACROLIB_H_
#define LITHIUM_MACROLIB_H_

/**
 * Lithium Macrolib
 * ================
 * Common macros, for creating macros, and for usage in code. Common
 * macros which may be defined by other headers (outside of Lithium)
 * will be undefined first.
 */

/**
 * Macro Construction Macros
 * -------------------------
 */

/**
 * Takes any amount of arguments and produces nothing.
 */
#undef EMPTY
#define EMPTY(...)

/**
 * Takes no arguments and produces a comma.
 */
#undef COMMA
#define COMMA() ,

/**
 * Takes no arguments and produces a semicolon.
 */
#undef SEMICOLON
#define SEMICOLON() ;

/**
 * Expand and stringify a token.
 */
#undef STRINGIFY
#define STRINGIFY(token) #token

/**
 * Expand and concatenate two tokens.
 */
#undef CONCAT2
#define CONCAT2(t1, t2) _LI_CONCAT2(t1, t2)
#define _LI_CONCAT2(t1, t2) t1##t2

/**
 * Expand, stringify, and place a comma at the end of a token.
 */
#define STRINGIFY_C(token) #token,

/**
 * Expand and place a comma at the end of a token.
 */
#define PASTE_C(token) token,

/**
 * Preprocessor Lists (``PPLIST``)
 * -------------------------------
 * Preprocessor lists are an effective way to manage data lists using
 * the preprocessor. The standard way to define a list is to define
 * macro of one argument ``M``, and apply a macro ``M`` to each list
 * element. For example::
 *
 *     #define SENSORS_PPLIST(M) \
 *             M(ACCEL_SENSOR)   \
 *             M(GYRO_SENSOR)    \
 *             M(TEMP_SENSOR)
 */

/**
 * Convert a ``PPLIST`` to a stringified and comma-separated
 * list. Example usage::
 *
 *     const char *const sensor_pretty_print[] =
 *             PPLIST_STRINGIFY(SENSORS_PPLIST);
 */
#define PPLIST_STRINGIFY(pplist)    \
	{                           \
		pplist(STRINGIFY_C) \
	}

/**
 * Convert a ``PPLIST`` to a comma-separated list. Example usage::
 *
 *     enum sensor_type PPLIST_PASTE(SENSORS_PPLIST);
 */
#define PPLIST_PASTE(pplist)    \
	{                       \
		pplist(PASTE_C) \
	}

/**
 * Take advantage of division by zero to indicate a failure condition.
 *
 * :param value: The value to return.
 * :param condition: The condition to assert at compile time.
 * :return: The value passed as ``value``.
 *
 * If usage is guaranteed to be inside of a function, a braced group
 * is recommended instead.
 */
#define STATIC_ASSERT_INLINE(value, condition) ((value) / !!(condition))

/**
 * Get the number of elements in an array.
 *
 * :param arr: An array.
 * :return: The number of elements in the array.
 *
 * For type safety, this function will cause a division-by-zero
 * compile-time check failure if a pointer is passed instead of an
 * array.
 */
#define ARRAY_SIZE(arr)                                                 \
	STATIC_ASSERT_INLINE(sizeof(arr) / sizeof((arr)[0]),            \
			     !__builtin_types_compatible_p(typeof(arr), \
							   typeof(&(arr)[0])))

/**
 * Attributes
 * ==========
 * These are common attributes to be used on a function.
 */

/**
 * Allow a static function to go unused.
 */
#undef __maybe_unused
#define __maybe_unused __attribute__((unused))

/**
 * Run this function before ``main`` runs.
 */
#undef __constructor
#define __constructor __attribute__((constructor))

/**
 * Run this function after ``main`` runs.
 */
#undef __destructor
#define __destructor __attribute__((destructor))

/**
 * Discard this function at link time.
 */
#undef __discard
#define __discard __attribute__((section("/DISCARD/")))

/**
 * This function does not return.
 */
#undef __noreturn
#define __noreturn __attribute__((noreturn))

/**
 * Report an error message if this function is used.
 *
 * :param msg: The message to show as a compiler error.
 *
 * Due to limitations with Clang, this is equivalent to
 * :c:macro:`__discard` on Clang.
 */
#undef __error_if_used
#ifdef __GNUC__
#define __error_if_used(msg) __attribute__((error(msg)))
#else
#define __error_if_used(msg) __discard
#endif

/* Make __auto_type available in C++ */
#if defined(__cplusplus) && !defined(__auto_type)
#define __auto_type auto
#endif

#endif /* LITHIUM_MACROLIB_H_ */
