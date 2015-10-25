/* util.h
 * PPP over Any Transport -- Helper routines
 *
 * Copyright (C) 2012-2015 Dmitry Podgorny <pasis.ua@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PPPOAT_UTIL_H__
#define __PPPOAT_UTIL_H__

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <sys/select.h>

#define PPPOAT_TIME_NEVER ULONG_MAX

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

/* FIXME: rewrite this, the arguments are evaluated twice */
#define pppoat_max(x, y) ((x) > (y) ? (x) : (y))

int pppoat_util_fd_nonblock_set(int fd, bool set);

int pppoat_util_select(int maxfd, fd_set *rfds, fd_set *wfds);
int pppoat_util_select_timed(int            maxfd,
			     fd_set        *rfds,
			     fd_set        *wfds,
			     unsigned long  usec);

int pppoat_util_write(int fd, void *buf, size_t len);
int pppoat_util_write_fd(int dst, int src);

#endif /* __PPPOAT_UTIL_H__ */
