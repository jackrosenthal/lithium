/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef LITHIUM_UTIL_REALLOCATING_BUFFER_H_
#define LITHIUM_UTIL_REALLOCATING_BUFFER_H_

#include <stddef.h>

struct li_reallocating_buffer {
	void *buf;
	size_t buf_usage;
	size_t buf_allocation;
};

ssize_t li_reallocating_buffer_read(int fd, struct li_reallocating_buffer *buf,
				    size_t n_bytes);

#endif /* LITHIUM_UTIL_REALLOCATING_BUFFER_H_ */
