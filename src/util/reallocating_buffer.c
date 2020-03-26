/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "util/reallocating_buffer.h"

#define MIN_REALLOC_SIZE (1 << 13) /* 8 KB */

static void increase_buffer_size(struct li_reallocating_buffer *buf,
				 size_t n_bytes)
{
	if (!n_bytes)
		return;

	buf->buf_allocation =
		((((buf->buf_allocation + n_bytes) / MIN_REALLOC_SIZE) + 1)) *
		MIN_REALLOC_SIZE;
	buf->buf = realloc(buf->buf, buf->buf_allocation);
}

ssize_t li_reallocating_buffer_read(int fd, struct li_reallocating_buffer *buf,
				    size_t n_bytes)
{
	if (buf->buf_allocation < buf->buf_usage)
		return -1;

	size_t space_left = buf->buf_allocation - buf->buf_usage;

	if (space_left < n_bytes)
		increase_buffer_size(buf, n_bytes - space_left);

	/* If fd is non-blocking, may as well increase the size of the
	   read to what we can handle */
	space_left = buf->buf_allocation - buf->buf_usage;
	if (space_left > n_bytes) {
		int flags = fcntl(fd, F_GETFL);
		if (flags > 0 && (flags & O_NONBLOCK))
			n_bytes = space_left;
	}

	ssize_t read_rv = read(fd, buf->buf + buf->buf_usage, n_bytes);
	if (read_rv > 0)
		buf->buf_usage += read_rv;

	return read_rv;
}
