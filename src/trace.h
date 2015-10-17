/* trace.h
 * PPP over Any Transport -- Tracing, asserts, core dumps
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

#ifndef __PPPOAT_TRACE_H__
#define __PPPOAT_TRACE_H__

#include <stdbool.h>
#include <stdlib.h>

#include "log.h"

#ifdef NDEBUG
#define PPPOAT_ASSERT(expr) do {} while (0)
#else /* NDEBUG */

#define PPPOAT_ASSERT(expr)                                                  \
	do {                                                                 \
		bool __expr = (expr);                                        \
		if (!__expr) {                                               \
			pppoat_fatal("trace",                                \
				     "%s:%d: %d: Assertion `%s' failed",     \
				     __FILE__, __LINE__, __func__, # expr);  \
			abort();                                             \
		}                                                            \
	} while (0)

#endif /* NDEBUG */

#endif /* __PPPOAT_TRACE_H__ */
