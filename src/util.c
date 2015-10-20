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
