/* log.h
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

#ifndef __PPPOAT_LOG_H__
#define __PPPOAT_LOG_H__

typedef enum {
	PPPOAT_DEBUG,
	PPPOAT_INFO,
	PPPOAT_ERROR,
	PPPOAT_FATAL,
	PPPOAT_LOG_LEVEL_NR,
} pppoat_log_level_t;

void pppoat_log_init(pppoat_log_level_t level);
void pppoat_log_fini(void);

void pppoat_log(pppoat_log_level_t level, const char *area,
		const char *fmt, ...);

#ifdef NDEBUG
#define pppoat_debug(area, fmt, ...) do {} while (0)
#else  /* NDEBUG */
#define pppoat_debug(area, fmt, ...) \
	pppoat_log(PPPOAT_DEBUG, area, fmt, ## __VA_ARGS__)
#endif /* NDEBUG */

#define pppoat_info(area, fmt, ...) \
	pppoat_log(PPPOAT_INFO, area, fmt, ## __VA_ARGS__)
#define pppoat_error(area, fmt, ...) \
	pppoat_log(PPPOAT_ERROR, area, fmt, ## __VA_ARGS__)
#define pppoat_fatal(area, fmt, ...) \
	pppoat_log(PPPOAT_FATAL, area, fmt, ## __VA_ARGS__)

#endif /* __PPPOAT_LOG_H__ */
