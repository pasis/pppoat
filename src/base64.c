#include <string.h>
#include <stdlib.h>

#include "base64.h"

const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		    "abcdefghijklmnopqrstuvwxyz"
		    "0123456789+/";

char *b64_encode(char *buf, size_t len)
{
	char *b64, *p;
	size_t new_len;
	int i;
	unsigned char t;
	unsigned char *buf2 = (unsigned char *) buf;

	new_len = (len + 2) / 3 * 4 + 1;
	b64 = p = (char *)malloc(new_len * sizeof(*p));
	if (!b64)
		return NULL;

	for (i = 0; i < len / 3; i++) {
		p[0] = cb64[buf2[0] >> 2];
		t = buf2[0] << 4 & 0x3f;
		p[1] = cb64[buf2[1] >> 4 | t];
		t = buf2[1] << 2 & 0x3f;
		p[2] = cb64[buf2[2] >> 6 | t];
		p[3] = cb64[buf2[2] & 0x3f];
		p += 4;
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

	*p = 0;

	return b64;
}

char *b64_decode(char *buf, size_t *p_len)
{
	char *b64, *p, *chr;
	size_t new_len, len;
	int i;
	unsigned char t;

	len = strlen(buf);
	new_len = len / 4 * 3;
	b64 = p = (char *)malloc(new_len * sizeof(*p));
	if (!b64)
		return NULL;

	for(i = 0; i < len; i++) {
		chr = strchr(cb64, (int) *buf++);
		if (!chr)
			break;
		t = (unsigned char) (chr - cb64);
		switch (i % 4) {
		case 0:
			*p = t << 2;
			break;
		case 1:
			*p++ |= t >> 4;
			*p = t << 4;
			break;
		case 2:
			*p++ |= t >> 2;
			*p = t << 6;
			break;
		case 3:
			*p++ |= t;
			break;
		}
	}

	*p_len = (size_t)(p - b64);
	return b64;
}
