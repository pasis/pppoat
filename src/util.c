/* util.c
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

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "trace.h"
#include "util.h"
#include "log.h"

int pppoat_util_fd_nonblock_set(int fd, bool set)
{
	int rc;

	rc = fcntl(fd, F_GETFL, NULL);
	if (rc >= 0) {
		/* rc contains flags */
		rc = set ? (rc | O_NONBLOCK) : (rc & (~O_NONBLOCK));
		rc = fcntl(fd, F_SETFL, rc);
	}
	rc = rc == -1 ? P_ERR(-errno) : 0;

	return rc;
}

int pppoat_util_select_timed(int            maxfd,
			     fd_set        *rfds,
			     fd_set        *wfds,
			     unsigned long  usec)
{
	struct timeval tv;
	int            rc;

	do {
		if (usec != PPPOAT_TIME_NEVER) {
			tv.tv_sec  = usec / 1000000;
			tv.tv_usec = usec % 1000000;
		}
		rc = select(maxfd + 1, rfds, wfds, NULL,
			    usec == PPPOAT_TIME_NEVER ? NULL : &tv);
	} while (rc < 0 && errno == EINTR);
	rc = rc < 0 ? P_ERR(-errno) : rc;

	return rc;
}

int pppoat_util_select(int maxfd, fd_set *rfds, fd_set *wfds)
{
	int rc;

	rc = pppoat_util_select_timed(maxfd, rfds, wfds, PPPOAT_TIME_NEVER);
	PPPOAT_ASSERT(rc != 0);

	return rc;
}

static bool util_error_is_recoverable(int error)
{
	return error == -EWOULDBLOCK ||
	       error == -EAGAIN      ||
	       error == -EINTR;
}

int pppoat_util_write(int fd, void *buf, size_t len)
{
	ssize_t nlen = (ssize_t)len;
	ssize_t wlen;
	fd_set  wfds;
	int     rc = 0;

	do {
		wlen = write(fd, buf, nlen);
		if (wlen < 0 && errno == EINTR)
			continue;
		if (wlen < 0 && !util_error_is_recoverable(-errno))
			rc = P_ERR(-errno);
		if (wlen < 0 && util_error_is_recoverable(-errno)) {
			FD_ZERO(&wfds);
			FD_SET(fd, &wfds);
			rc = pppoat_util_select(fd, NULL, &wfds);
		}
		if (wlen > 0) {
			buf   = (char *)buf + wlen;
			nlen -= wlen;
		}
	} while (rc == 0 && nlen > 0);

	return rc;
}

int pppoat_util_write_fd(int dst, int src)
{
	unsigned char buf[4096]; /* FIXME: avoid buffer on stack */
	ssize_t       len;
	int           rc = 0;

	/* TODO: use splice(2) on Linux */
	do {
		len = read(src, buf, sizeof(buf));
		if (len < 0 && errno == EINTR)
			continue;
		if (len < 0 && !util_error_is_recoverable(-errno))
			rc = P_ERR(-errno);
		if (len == 0)
			rc = P_ERR(-EPIPE); /* FIXME: return EOF somehow */
		if (len > 0)
			rc = pppoat_util_write(dst, buf, len);
	} while (false);

	return rc;
}
