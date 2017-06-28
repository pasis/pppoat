/* memory.c
 * PPP over Any Transport -- Allocator
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

#include <stdlib.h>	/* malloc, free */
#include <string.h>	/* memset */

#include "memory.h"

void *pppoat_alloc(size_t size)
{
	return malloc(size);
}

void *pppoat_calloc(size_t nmemb, size_t size)
{
	size_t  arr_size = nmemb * size;
	void   *ptr      = pppoat_alloc(arr_size);

	if (ptr != NULL)
		memset(ptr, 0, arr_size);
	return ptr;
}

void pppoat_free(void *ptr)
{
	free(ptr);
}

char *pppoat_strdup(const char *s)
{
	/* TODO implement via pppoat_alloc() */
	return strdup(s);
}
