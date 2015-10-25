/* log.c
 * PPP over Any Transport -- Logging
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

#include <stdarg.h>
#include <stdio.h>

#include "log.h"

static pppoat_log_level_t log_level_min = PPPOAT_LOG_LEVEL_NR;

static const char *log_level_name_tbl[PPPOAT_LOG_LEVEL_NR] = {
	[PPPOAT_DEBUG] = "DEBUG",
	[PPPOAT_INFO]  = "INFO ",
	[PPPOAT_ERROR] = "ERROR",
	[PPPOAT_FATAL] = "FATAL",
};

static const char *log_level_name(pppoat_log_level_t level)
{
	return level < PPPOAT_LOG_LEVEL_NR ?
	       log_level_name_tbl[level] : "NONE";
}

void pppoat_log_init(pppoat_log_level_t level)
{
	log_level_min = level;
}

void pppoat_log_fini(void)
{
	log_level_min = PPPOAT_LOG_LEVEL_NR;
}

void pppoat_log(pppoat_log_level_t level, const char *area,
		const char *fmt, ...)
{
	const char *slevel = log_level_name(level);
	va_list     ap;

	if (level < log_level_min)
		return;

	va_start(ap, fmt);

	fprintf(stderr, "%s %s: ", slevel, area);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");

	va_end(ap);
}
