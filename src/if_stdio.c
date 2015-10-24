/* if_stdio.c
 * PPP over Any Transport -- stdin/stdout instead of network interface
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
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "trace.h"
#include "if_stdio.h"
#include "if.h"
#include "log.h"
#include "memory.h"

#define PPPOAT_STDIO_TIMEOUT 1000000

struct stdio_ctx {
	int        sc_rd;
	int        sc_wr;
	pthread_t  sc_thread;
	sem_t      sc_stop;
};

static int if_module_stdio_init(int argc, char **argv, void **userdata)
{
	struct stdio_ctx *ctx;
	int               rc;

	ctx = pppoat_alloc(sizeof(*ctx));
	PPPOAT_ASSERT(ctx != NULL);
	rc = sem_init(&ctx->sc_stop, 0, 0);
	PPPOAT_ASSERT(rc == 0);

	*userdata = ctx;

	return 0;
}

static void if_module_stdio_fini(void *userdata)
{
	struct stdio_ctx *ctx = userdata;

	sem_destroy(&ctx->sc_stop);
	pppoat_free(ctx);
}

static void *stdio_thread(void *userdata)
{
	struct stdio_ctx *ctx = userdata;
	struct timeval    tv;
	unsigned long     timeout;
	unsigned char     buf[4096]; /* TODO: alloc during init */
	ssize_t           len;
	ssize_t           len2;
	fd_set            rfds;
	int               rc;

	timeout = PPPOAT_STDIO_TIMEOUT; /* XXX */

	while (sem_trywait(&ctx->sc_stop) != 0) {
		tv.tv_sec  = timeout / 1000000;
		tv.tv_usec = timeout % 1000000;
		FD_ZERO(&rfds);
		FD_SET(ctx->sc_rd, &rfds);
		FD_SET(0, &rfds);
		do {
			rc = select(ctx->sc_rd + 1, &rfds, NULL, NULL, &tv);
		} while (rc < 0 && errno == EINTR);
		PPPOAT_ASSERT(rc >= 0);

		if (FD_ISSET(ctx->sc_rd, &rfds)) {
			len = read(ctx->sc_rd, buf, sizeof(buf));
			PPPOAT_ASSERT(len >= 0);
			len2 = write(1, buf, len);
			PPPOAT_ASSERT_INFO(len2 == len, "len2=%zd", len2);
		}
		if (FD_ISSET(0, &rfds)) {
			len = read(0, buf, sizeof(buf));
			PPPOAT_ASSERT(len >= 0);
			if (len == 0)
				break;
			len2 = write(ctx->sc_wr, buf, len);
			PPPOAT_ASSERT_INFO(len2 == len, "len2=%zd", len2);
		}
	}
	close(ctx->sc_rd);
	close(ctx->sc_wr);

	return NULL;
}

static int if_module_stdio_run(int rd, int wr, void *userdata)
{
	struct stdio_ctx *ctx = userdata;
	int               rc;

	ctx->sc_rd = dup(rd);
	ctx->sc_wr = dup(wr);
	PPPOAT_ASSERT(ctx->sc_rd != -1);
	PPPOAT_ASSERT(ctx->sc_wr != -1);
	rc = pthread_create(&ctx->sc_thread, NULL, &stdio_thread, userdata);
	PPPOAT_ASSERT(rc == 0);

	return 0;
}

static int if_module_stdio_stop(void *userdata)
{
	struct stdio_ctx *ctx = userdata;
	int               rc;

	sem_post(&ctx->sc_stop);
	rc = pthread_join(ctx->sc_thread, NULL);
	PPPOAT_ASSERT(rc == 0);

	return 0;
}

const struct pppoat_if_module pppoat_if_module_stdio = {
	.im_name  = "stdio",
	.im_descr = "Using stdin/stdout instead of network interface",
	.im_init  = &if_module_stdio_init,
	.im_fini  = &if_module_stdio_fini,
	.im_run   = &if_module_stdio_run,
	.im_stop  = &if_module_stdio_stop,
};
