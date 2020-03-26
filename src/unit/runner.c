/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "constants.h"
#include "unit.h"
#include "util/timespec.h"

struct li_unit_test *li_unit_test_list = NULL;

static struct {
	unsigned int completed_tests;
	unsigned int total_tests;
	int running_jobs;
	int notify_pipe[2];
	struct timespec status_update_deadline;
	struct li_unit_test *remaining_tests;
} runner_state;

static const char *test_state_pretty_print[] = {
	[_LI_UNIT_NOT_STARTED] = "NOT STARTED",
	[_LI_UNIT_RUNNING] = "RUNNING",
	[_LI_UNIT_PENDING_DEADLINE_EXCEEDED] = "PENDING DEADLINE EXCEEDED",
	[_LI_UNIT_SUCCEEDED] = "SUCCEEDED",
	[_LI_UNIT_FAILED] = "FAILED",
	[_LI_UNIT_DEADLINE_EXCEEDED] = "DEADLINE EXCEEDED",
};

static int spawn_test(int default_timeout)
{
	struct li_unit_test *test = runner_state.remaining_tests;

	if (test->priv.state != _LI_UNIT_NOT_STARTED) {
		fprintf(stderr, "Invalid state transition: %s -> %s\n",
			test_state_pretty_print[test->priv.state],
			test_state_pretty_print[test->priv.state]);
		return -1;
	}
	test->priv.state = _LI_UNIT_RUNNING;

	runner_state.remaining_tests = runner_state.remaining_tests->rest;
	runner_state.running_jobs++;

	if (clock_gettime(CLOCK_MONOTONIC, &test->priv.start_time) < 0) {
		perror("clock_gettime failed");
		return -1;
	}

	pid_t pid = fork();
	if (pid < 0) {
		perror("fork failed");
		return -1;
	}

	if (pid == 0) {
		/* child process */
		li_unit_run_test(test);
	}

	test->priv.pid = pid;

	/* compute the deadline for the test */
	int timeout_multiplier = test->options.timeout_multiplier;

	if (timeout_multiplier == 0)
		timeout_multiplier = 1;

	if (timeout_multiplier > 0 && default_timeout > 0) {
		test->priv.has_deadline = true;
		test->priv.deadline.tv_nsec = test->priv.start_time.tv_nsec;
		test->priv.deadline.tv_sec =
			test->priv.start_time.tv_sec +
			(timeout_multiplier * default_timeout);
	} else {
		test->priv.has_deadline = false;
	}

	return 0;
}

static int handle_waitpid(struct li_unit_test *test_list, pid_t pid, int status)
{
	struct li_unit_test *test = test_list;

	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
		perror("clock_gettime failed");
		return -1;
	}

	while (test && test->priv.pid != pid)
		test = test->rest;

	if (!test) {
		fprintf(stderr, "No running test with pid %d found.\n", pid);
		return -1;
	}

	if (!(test->priv.state == _LI_UNIT_RUNNING ||
	      _LI_UNIT_PENDING_DEADLINE_EXCEEDED)) {
		fprintf(stderr, "Invalid state transition: %s -> %s\n",
			test_state_pretty_print[test->priv.state],
			test_state_pretty_print[test->priv.state]);
		return -1;
	}

	runner_state.running_jobs--;
	li_util_timespec_subtract(&now, &test->priv.start_time,
				  &test->priv.elapsed_time);
	runner_state.completed_tests++;

	const char *reason;

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		test->priv.state = _LI_UNIT_SUCCEEDED;
		reason = "succeeded";
	} else if (test->priv.state == _LI_UNIT_PENDING_DEADLINE_EXCEEDED) {
		test->priv.state = _LI_UNIT_DEADLINE_EXCEEDED;
		reason = "timed out";
	} else {
		test->priv.state = _LI_UNIT_FAILED;
		reason = "failed";
	}

	fprintf(stderr, "[%3u/%u] %s %s! (%lld.%03lds)\n",
		runner_state.completed_tests, runner_state.total_tests,
		test->name, reason, (long long)test->priv.elapsed_time.tv_sec,
		test->priv.elapsed_time.tv_nsec / NSEC_PER_MSEC);

	return 0;
}

static int handle_deadline(struct li_unit_test *test)
{
	if (kill(test->priv.pid, SIGKILL) < 0) {
		perror("kill failed");
		return -1;
	}

	test->priv.state = _LI_UNIT_PENDING_DEADLINE_EXCEEDED;
	return 0;
}

static int handle_status_update(struct li_unit_test *test_list,
				unsigned int status_update_frequency)
{
	fprintf(stderr, "\nPending tasks:\n");

	for (struct li_unit_test *t = test_list; t; t = t->rest) {
		if (t->priv.state < _LI_UNIT_SUCCEEDED)
			fprintf(stderr, "  %s (%s)\n", t->name,
				test_state_pretty_print[t->priv.state]);
	}
	fprintf(stderr, "\n");

	if (clock_gettime(CLOCK_MONOTONIC,
			  &runner_state.status_update_deadline) < 0) {
		perror("clock_gettime failed");
		return -1;
	}

	runner_state.status_update_deadline.tv_sec += status_update_frequency;
	return 0;
}

static enum {
	TEST_RUNNER_ITERATE_AGAIN,
	TEST_RUNNER_ITERATE_FAILURE,
	TEST_RUNNER_ITERATE_SUCCESS,
} test_runner_iterate(struct li_unit_runner_options *options)
{
	/* Used for a read from a pipe we don't care about the data */
	char unused_buf[4096];
	int status;
	pid_t pid;

	if (!runner_state.running_jobs && !runner_state.remaining_tests)
		return TEST_RUNNER_ITERATE_SUCCESS;

	if (runner_state.running_jobs < options->parallelism &&
	    runner_state.remaining_tests) {
		if (spawn_test(options->default_timeout) < 0) {
			fprintf(stderr, "spawn_test failed!\n");
			return TEST_RUNNER_ITERATE_FAILURE;
		}
		return TEST_RUNNER_ITERATE_AGAIN;
	}

	if (read(runner_state.notify_pipe[0], unused_buf, sizeof(unused_buf)) >
	    0) {
		return TEST_RUNNER_ITERATE_AGAIN;
	}

	pid = waitpid(-1, &status, WNOHANG);
	if (pid < 0) {
		perror("waitpid failed");
		return TEST_RUNNER_ITERATE_FAILURE;
	}

	if (pid > 0) {
		if (handle_waitpid(options->test_list, pid, status) < 0) {
			fprintf(stderr, "handle_waitpid failed!\n");
			return TEST_RUNNER_ITERATE_FAILURE;
		}
		return TEST_RUNNER_ITERATE_AGAIN;
	}

	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
		perror("clock_gettime failed");
		return TEST_RUNNER_ITERATE_FAILURE;
	}

	if (now.tv_sec >= runner_state.status_update_deadline.tv_sec &&
	    now.tv_nsec >= runner_state.status_update_deadline.tv_nsec) {
		if (handle_status_update(options->test_list,
					 options->status_update_frequency) <
		    0) {
			fprintf(stderr, "failed to print status update!\n");
			return TEST_RUNNER_ITERATE_FAILURE;
		}
		return TEST_RUNNER_ITERATE_AGAIN;
	}

	struct timespec *next_deadline = &runner_state.status_update_deadline;
	for (struct li_unit_test *test = options->test_list;
	     test != runner_state.remaining_tests; test = test->rest) {
		if (test->priv.state == _LI_UNIT_RUNNING &&
		    test->priv.has_deadline &&
		    li_util_timespec_lt(&test->priv.deadline, &now)) {
			handle_deadline(test);
			return TEST_RUNNER_ITERATE_AGAIN;
		} else if (test->priv.state == _LI_UNIT_RUNNING &&
			   test->priv.has_deadline &&
			   li_util_timespec_lt(&test->priv.deadline,
					       next_deadline)) {
			next_deadline = &test->priv.deadline;
		}
	}

	struct timespec timeout;
	li_util_timespec_subtract(next_deadline, &now, &timeout);

	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(runner_state.notify_pipe[0], &rfds);
	if (pselect(runner_state.notify_pipe[0] + 1, &rfds, NULL, NULL,
		    &timeout, NULL) < 0) {
		if (errno == EINTR)
			return TEST_RUNNER_ITERATE_AGAIN;

		perror("pselect failed");
		return TEST_RUNNER_ITERATE_FAILURE;
	}

	return TEST_RUNNER_ITERATE_AGAIN;
}

static void sigchld_handler(int sig)
{
	/* Save the errno so we can restore it. */
	int previous_errno = errno;

	/* Write one byte to the notify pipe so that the pselect
	   returns. Then, the waitpid will be picked up on the next
	   iteration of the test runner. */
	(void)write(runner_state.notify_pipe[1], "", 1);

	errno = previous_errno;
}

static void print_failure_output(struct li_unit_test *test)
{
	char top_output[80];
	char bottom_output[sizeof(top_output)];
	const char *reason = "failed";
	const char *informational_str = "";

	memset(top_output, '=', sizeof(top_output) - 1);
	top_output[sizeof(top_output) - 1] = '\0';
	memcpy(bottom_output, top_output, sizeof(bottom_output));

	if (test->options.informational)
		informational_str = ", informational";

	if (test->priv.state == _LI_UNIT_DEADLINE_EXCEEDED)
		reason = "timed out";

	snprintf(top_output, sizeof(top_output) - 1, "== %s (%s%s) ",
		 test->name, reason, informational_str);

	/* Remove null byte from snprintf */
	for (size_t i = 0; i < sizeof(top_output) - 1; i++) {
		if (!top_output[i])
			top_output[i] = '=';
	}
	fprintf(stderr, "%s\n", top_output);
	fprintf(stderr, "TODO: OUTPUT GOES HERE\n");
	fprintf(stderr, "%s\n", bottom_output);
}

int li_unit_run_tests(struct li_unit_runner_options *options)
{
	if (!options->default_timeout)
		options->default_timeout = 10;
	if (!options->parallelism)
		options->parallelism = get_nprocs();
	if (!options->status_update_frequency)
		options->status_update_frequency = 15;
	if (!options->test_list)
		options->test_list = li_unit_test_list;

	unsigned int failures = 0;
	unsigned int informational_failures = 0;

	memset(&runner_state, 0, sizeof(runner_state));

	runner_state.running_jobs = 0;
	runner_state.remaining_tests = options->test_list;

	for (struct li_unit_test *test = options->test_list; test;
	     test = test->rest) {
		runner_state.total_tests++;
	}

	fprintf(stderr, "Running %u tests with a parallelism of %u.\n",
		runner_state.total_tests, options->parallelism);

	if (pipe(runner_state.notify_pipe) < 0) {
		perror("pipe failed");
		return -1;
	}

	for (int i = 0; i < ARRAY_SIZE(runner_state.notify_pipe); i++) {
		int flags = fcntl(runner_state.notify_pipe[i], F_GETFL);
		if (flags < 0) {
			perror("fcntl failed");
			return -1;
		}
		if (fcntl(runner_state.notify_pipe[i], F_SETFL,
			  flags | O_NONBLOCK) < 0) {
			perror("fcntl failed");
			return -1;
		}
	}

	if (clock_gettime(CLOCK_MONOTONIC,
			  &runner_state.status_update_deadline) < 0) {
		perror("clock_gettime failed");
		return -1;
	}
	runner_state.status_update_deadline.tv_sec +=
		options->status_update_frequency;

	struct sigaction sa;
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &sa, 0) < 0) {
		perror("setting up SIGCHLD handler failed");
		return -1;
	}

	for (;;) {
		switch (test_runner_iterate(options)) {
		case TEST_RUNNER_ITERATE_AGAIN:
			continue;
		case TEST_RUNNER_ITERATE_SUCCESS:
			goto exit;
		default:
			return -1;
		}
	}

exit:
	fprintf(stderr, "\n");

	for (struct li_unit_test *test = options->test_list; test;
	     test = test->rest) {
		if (test->priv.state != _LI_UNIT_SUCCEEDED) {
			if (test->options.informational)
				informational_failures++;
			else
				failures++;

			print_failure_output(test);
		}
	}

	if (informational_failures == 0 && failures == 0)
		fprintf(stderr, "All tests passed!\n");
	else if (informational_failures != 0 && failures == 0)
		fprintf(stderr,
			"\nSuccess, all failing tests are informational!\n");
	else
		fprintf(stderr, "\nYou have failing tests!\n");

	return failures > 0;
}
