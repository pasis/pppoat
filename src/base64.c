/* base64.c
 * PPP over Any Transport -- Base64
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
#include <string.h>

#include "trace.h"
#include "base64.h"
#include "memory.h"

/* The Base 64 Alphabet, padding is '=' */
static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			   "abcdefghijklmnopqrstuvwxyz"
			   "0123456789+/";

size_t pppoat_base64_enc_len(const void *buf, size_t len)
{
	return (len + 2) / 3 * 4;
}

void pppoat_base64_enc(const void *buf,
		       size_t      len,
		       char       *result,
		       size_t      result_len)
{
	const unsigned char *buf2 = buf;
	unsigned char        t;
	char                *p = result;
	int                  i;

	PPPOAT_ASSERT(result_len == pppoat_base64_enc_len(buf, len));

	for (i = 0; i < len / 3; ++i) {
		p[0] = cb64[buf2[0] >> 2];
		t    = buf2[0] << 4 & 0x3f;
		p[1] = cb64[buf2[1] >> 4 | t];
		t    = buf2[1] << 2 & 0x3f;
		p[2] = cb64[buf2[2] >> 6 | t];
		p[3] = cb64[buf2[2] & 0x3f];
		p    += 4;
		buf2 += 3;
	}

	if (len % 3) {
		*p++ = cb64[*buf2 >> 2];
		t = *buf2++ << 4 & 0x3f;
		if (len % 3 == 2) {
			*p++ = cb64[*buf2 >> 4 | t];
			*p++ = cb64[*buf2 << 2 & 0x3f];
		} else
			*p++ = cb64[t];
		for (i = 0; i < 3 - len % 3; i++)
			*p++ = '=';
	}
}

size_t pppoat_base64_dec_len(const char *base64, size_t len)
{
	size_t data_len;

	PPPOAT_ASSERT(len % 4 == 0);
	data_len = len / 4 * 3;
	if (len > 0 && base64[len - 1] == '=')
		--data_len;
	if (len > 1 && base64[len - 2] == '=')
		--data_len;

	return data_len;
}

void pppoat_base64_dec(const char *base64,
		       size_t      len,
		       void       *result,
		       size_t      result_len)
{
	unsigned char *p   = result;
	const char    *buf = base64;
	unsigned char  t;
	char          *c;
	int            i;

	PPPOAT_ASSERT(len % 4 == 0);
	PPPOAT_ASSERT(result_len == pppoat_base64_dec_len(base64, len));

	for(i = 0; i < len; i++) {
		c = strchr(cb64, (int) *buf++);
		PPPOAT_ASSERT(c != NULL);
		t = (unsigned char)(c - cb64);
		PPPOAT_ASSERT((t & 0xc0) == 0);
		switch (i % 4) {
		case 0:
			*p = t << 2;
			break;
		case 1:
			*p++ |= t >> 4;
			*p    = t << 4;
			break;
		case 2:
			*p++ |= t >> 2;
			*p    = t << 6;
			break;
		case 3:
			*p++ |= t;
			break;
		}
	}
}

bool pppoat_base64_is_valid(const char *base64, size_t len)
{
	size_t i;
	bool   valid;

	valid = len % 4 == 0;
	if (valid) {
		if (len != 0) {
			len = base64[len - 1] == '=' ? len - 1 : len;
			len = base64[len - 1] == '=' ? len - 1 : len;
		}
		for (i = 0; valid && i < len; ++i)
			valid = strchr(cb64, (int)base64[i]) != NULL;
	}
	return valid;
}

int pppoat_base64_enc_new(const void *buf,
			  size_t      len,
			  char      **result)
{
	size_t rlen = pppoat_base64_enc_len(buf, len) + 1;
	int    rc;

	*result = pppoat_alloc(rlen);
	rc = *result == NULL ? -ENOMEM : 0;
	if (rc == 0) {
		pppoat_base64_enc(buf, len, *result, rlen);
		*result[rlen - 1] = '\0';
	}
	return rc;
}

int pppoat_base64_dec_new(const char     *base64,
			  size_t          len,
			  unsigned char **result,
			  size_t         *result_len)
{
	size_t rlen = pppoat_base64_dec_len(base64, len);
	int    rc;

	rc = pppoat_base64_is_valid(base64, len) ? 0 : -EINVAL;
	if (rc == 0) {
		*result = pppoat_alloc(rlen);
		rc = *result == NULL ? -ENOMEM : 0;
	}
	if (rc == 0) {
		pppoat_base64_dec(base64, len, *result, rlen);
		*result_len = rlen;
	}
	return rc;
}
