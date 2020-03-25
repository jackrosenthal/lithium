/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "unit.h"

static char *parse_error_message;

void li_cmdline_set_parse_error(const char *message)
{
	static size_t allocation_size;
	size_t message_len = strlen(message) + 1;

	if (message_len < allocation_size) {
		parse_error_message = realloc(parse_error_message,
					      message_len * sizeof(char));
		allocation_size = message_len;
	}

	memcpy(parse_error_message, message, message_len);
}

static bool is_opt(const struct li_cmdline_option *option)
{
	return option && (option->shortopt || option->longopt);
}

static bool parse_from_format(const char *format, const char *value, void *dest)
{
	/* Make a new format string with %c at the end, so we can
	   detect extra characters. */
	bool rv = true;
	size_t tmp_format_len = strlen(format) + 3;
	char *tmp_format = malloc(tmp_format_len * sizeof(char));
	snprintf(tmp_format, tmp_format_len, "%s%s", format, "%c");
	char unused;

	if (sscanf(value, tmp_format, dest, &unused) != 1)
		rv = false;

	free(tmp_format);
	return rv;
}

DEFTEST("lithium.cmdline.internals.parse_from_format",
	{ .informational = true })
{
	int int_dest;
	unsigned int uint_dest;

	EXPECT(parse_from_format("%d", "123", &int_dest));
	EXPECT(int_dest == 123);

	EXPECT(parse_from_format("%u", "123", &uint_dest));
	EXPECT(uint_dest == 123);

	EXPECT(!parse_from_format("%u", "-123", &uint_dest));
}

static bool complete_action(struct li_cmdline_action *action, const char *value)
{
	switch (action->type) {
	case LI_CMDLINE_STORE_TRUE:
		*(bool *)action->dest = true;
		return true;
	case LI_CMDLINE_STORE_FALSE:
		*(bool *)action->dest = false;
		return true;
	case LI_CMDLINE_STRING:
		*(const char **)action->dest = value;
		return true;
	case LI_CMDLINE_SCANF:
		return parse_from_format(action->format, value, action->dest);
	case LI_CMDLINE_CALLBACK:
		return action->cb(value, action->dest);
	default:
		return false;
	}
}

static void opt_help_text_left(struct li_cmdline_option *opt, char *buf,
			       size_t buf_sz)
{
	char capstring[100] = { 0 };
	if (opt->action.type > LI_CMDLINE_HELP) {
		snprintf(capstring, sizeof(capstring) - 1, " %s",
			 opt->longopt ? opt->longopt : "VALUE");
		for (size_t i = 0; capstring[i]; i++) {
			capstring[i] = toupper(capstring[i]);
		}
	}

	buf[buf_sz - 1] = '\0';
	buf_sz--;

	if (opt->shortopt && opt->longopt)
		snprintf(buf, buf_sz, "-%c%s, --%s%s", opt->shortopt, capstring,
			 opt->longopt, capstring);
	else if (opt->shortopt)
		snprintf(buf, buf_sz, "-%c%s", opt->shortopt, capstring);
	else
		snprintf(buf, buf_sz, "--%s%s", opt->longopt, capstring);
}

static void show_help_message(struct li_cmdline *spec, const char *program_name,
			      bool additional_args)
{
	if (spec->title)
		fprintf(stderr, "%s\n", spec->title);

	fprintf(stderr, "Usage: %s", program_name);
	if (is_opt(spec->options))
		fprintf(stderr, " [OPTIONS...]");
	if ((spec->arguments && spec->arguments[0].name) || additional_args)
		fprintf(stderr, " [--]");
	for (size_t i = 0; spec->arguments && spec->arguments[i].name; i++) {
		if (spec->arguments[i].optional)
			fprintf(stderr, " [%s]", spec->arguments[i].name);
		else
			fprintf(stderr, " %s", spec->arguments[i].name);
	}
	if (additional_args)
		fprintf(stderr, " ...");
	fprintf(stderr, "\n");

	int left_column_size = 0;
	for (size_t i = 0; spec->options && is_opt(spec->options + i); i++) {
		char buf[100];
		opt_help_text_left(&spec->options[i], buf, sizeof(buf));
		size_t this_len = strlen(buf);
		if (this_len > left_column_size)
			left_column_size = this_len;
	}

	for (size_t i = 0; spec->arguments && spec->arguments[i].name; i++) {
		size_t this_len = strlen(spec->arguments[i].name);
		if (this_len > left_column_size)
			left_column_size = this_len;
	}

	if (is_opt(spec->options))
		fprintf(stderr, "\nOptions:\n");
	for (size_t i = 0; spec->options && is_opt(spec->options + i); i++) {
		char buf[100];
		opt_help_text_left(&spec->options[i], buf, sizeof(buf));

		const char *help_text = spec->options[i].help;
		if (!help_text &&
		    spec->options[i].action.type == LI_CMDLINE_HELP)
			help_text = "Show this help message and exit.";
		if (!help_text)
			help_text = "Undocumented.";

		fprintf(stderr, "  %-*s %s\n", left_column_size, buf,
			help_text);
	}

	if (spec->arguments && spec->arguments[0].name)
		fprintf(stderr, "\nPositional Arguments:\n");
	for (size_t i = 0; spec->arguments && spec->arguments[i].name; i++) {
		const char *help_text = spec->arguments[i].help;
		if (!help_text)
			help_text = "Undocumented.";
		fprintf(stderr, "  %.*s %s\n", left_column_size,
			spec->arguments[i].name, help_text);
	}

	if (spec->help)
		fprintf(stderr, "\n%s\n", spec->help);
}

static enum li_cmdline_parse_result argparse(const char *program_name,
					     struct li_cmdline *spec,
					     const char *const argv[],
					     const char *const *argv_out[])
{
	for (size_t i = 0; spec->arguments && spec->arguments[i].name; i++) {
		if (!argv[0]) {
			if (spec->arguments[i].optional)
				return LI_CMDLINE_CONTINUE;
			show_help_message(spec, program_name, argv_out);
			fprintf(stderr,
				"\nMissing required positional argument: %s\n",
				spec->arguments[i].name);
			return LI_CMDLINE_EXIT_FAILURE;
		}
		if (!complete_action(&spec->arguments[i].action, argv[i])) {
			show_help_message(spec, program_name, argv_out);
			fprintf(stderr, "\nPositional argument %s: %s\n",
				spec->arguments[i].name,
				parse_error_message ? parse_error_message :
						      "invalid value.");
			return LI_CMDLINE_EXIT_FAILURE;
		}
		argv++;
	}

	if (argv_out) {
		*argv_out = argv;
	} else if (argv[0]) {
		show_help_message(spec, program_name, argv_out);
		fprintf(stderr, "\nToo many arguments!\n");
		return LI_CMDLINE_EXIT_FAILURE;
	}

	return LI_CMDLINE_CONTINUE;
}

static struct li_cmdline_option *
matchopt_long(struct li_cmdline *spec, const char *arg, const char **val_out)
{
	char *equals = strchr(arg, '=');
	if (equals) {
		equals[0] = '\0';
		*val_out = equals + 1;
	}

	for (size_t i = 0; spec->options && is_opt(spec->options + i); i++) {
		if (!strcmp(arg, spec->options[i].longopt))
			return &spec->options[i];
	}
	return NULL;
}

static struct li_cmdline_option *
matchopt_short(struct li_cmdline *spec, const char *arg, const char **val_out)
{
	if (!arg[0])
		return NULL;

	if (arg[1])
		*val_out = arg + 1;

	for (size_t i = 0; spec->options && is_opt(spec->options + i); i++) {
		if (arg[0] == spec->options[i].shortopt)
			return &spec->options[i];
	}
	return NULL;
}

static struct li_cmdline_option *matchopt(struct li_cmdline *spec,
					  const char *arg, const char **val_out)
{
	*val_out = NULL;
	if (!strncmp(arg, "--", 2))
		return matchopt_long(spec, arg + 2, val_out);
	return matchopt_short(spec, arg + 1, val_out);
}

static enum li_cmdline_parse_result optparse(const char *program_name,
					     struct li_cmdline *spec,
					     const char *const argv[],
					     const char *const *argv_out[])
{
	if (!argv[0] || argv[0][0] != '-')
		return argparse(program_name, spec, argv, argv_out);
	if (!strcmp(argv[0], "--"))
		return argparse(program_name, spec, argv + 1, argv_out);

	const char *flag = argv[0];
	const char *value;
	struct li_cmdline_option *option = matchopt(spec, flag, &value);
	if (!option) {
		show_help_message(spec, program_name, argv_out);
		fprintf(stderr, "\nUnrecognized option: %s\n", flag);
		return LI_CMDLINE_EXIT_FAILURE;
	}

	if (option->action.type == LI_CMDLINE_HELP) {
		show_help_message(spec, program_name, argv_out);
		return LI_CMDLINE_EXIT_SUCCESS;
	} else if (option->action.type >= LI_CMDLINE_STRING) {
		if (!value) {
			argv++;
			value = argv[0];
		}
		if (!value) {
			show_help_message(spec, program_name, argv_out);
			fprintf(stderr, "\n%s: missing value\n", flag);
			return LI_CMDLINE_EXIT_FAILURE;
		}
	} else {
		if (value) {
			fprintf(stderr,
				"Passing multiple flags in same arg is not "
				"supported.\n");
			return LI_CMDLINE_EXIT_FAILURE;
		}
	}

	if (!complete_action(&option->action, value)) {
		show_help_message(spec, program_name, argv_out);
		fprintf(stderr, "\n%s: %s\n", flag,
			parse_error_message ? parse_error_message :
					      "invalid value.");
		return LI_CMDLINE_EXIT_FAILURE;
	}

	return optparse(program_name, spec, argv + 1, argv_out);
}

enum li_cmdline_parse_result li_cmdline_parse(struct li_cmdline *spec,
					      const char *const argv[],
					      const char *const *argv_out[])
{
	if (argv_out)
		*argv_out = NULL;
	return optparse(argv[0], spec, argv + 1, argv_out);
}
