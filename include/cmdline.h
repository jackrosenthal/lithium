/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef LITHIUM_CMDLINE_H_
#define LITHIUM_CMDLINE_H_

#include <stdbool.h>

/**
 * Command Line Parsing Utilities
 * ==============================
 */

/**
 * A definition of a text parser and its destination.
 */
struct li_cmdline_action {
	enum {
		LI_CMDLINE_STORE_TRUE,
		LI_CMDLINE_STORE_FALSE,
		LI_CMDLINE_HELP,
		LI_CMDLINE_STRING,
		LI_CMDLINE_SCANF,
		LI_CMDLINE_CALLBACK,
	} type;
	union {
		const char *format;
		bool (*cb)(const char *value, void *dest);
	};
	void *dest;
};

/**
 * A struct representing a single command line option.
 */
struct li_cmdline_option {
	char shortopt;
	const char *longopt;
	const char *help;
	struct li_cmdline_action action;
};

/**
 * A struct representing a single positional argument.
 */
struct li_cmdline_argument {
	const char *name;
	const char *help;
	bool optional;
	struct li_cmdline_action action;
};

/**
 * A struct representing the options to a program.
 */
struct li_cmdline {
	/**
	 * A descriptive name of the program.
	 */
	const char *title;

	/**
	 * Additional help text.
	 */
	const char *help;

	/**
	 * NULL-terminated list of options.
	 */
	struct li_cmdline_option *options;

	/**
	 * NULL-terminated list of arguments.
	 */
	struct li_cmdline_argument *arguments;
};

enum li_cmdline_parse_result {
	LI_CMDLINE_EXIT_SUCCESS,
	LI_CMDLINE_EXIT_FAILURE,
	LI_CMDLINE_CONTINUE,
};

/**
 * Parse the command line.
 *
 * :param spec: The command line spec.
 * :param argv: The NULL-terminated argument list.
 * :param argv_out: If non-NULL, set this pointer to any remaining
 *                  arguments. If NULL, error when extra arguments are
 *                  given.
 * :return: :c:data:`LI_CMDLINE_CONTINUE` if the program should
 *          continue, :c:data:`LI_CMDLINE_EXIT_SUCCESS` if the program
 *          should exit with a successful status (e.g., "--help" was
 *          passed), or :c:data:`LI_CMDLINE_EXIT_FAILURE` if the
 *          program should exit with a non-zero exit status (e.g.,
 *          there was a parse error).
 */
enum li_cmdline_parse_result li_cmdline_parse(struct li_cmdline *spec,
					      const char *const argv[],
					      const char *const *argv_out[]);

/**
 * Set a parse error, to be used by parser callback functions.
 *
 * :param message: The error message. This can be stack allocated, as
 *                 the string is copied.
 *
 * Any previously set error messages will be overwritten.
 */
void li_cmdline_set_parse_error(const char *message);

#endif /* LITHIUM_CMDLINE_H_ */
