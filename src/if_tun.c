/* if_tun.c
 * PPP over Any Transport -- TUN/TAP network interface module
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
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "trace.h"
#include "conf.h"
#include "if_tun.h"
#include "if.h"
#include "log.h"
#include "memory.h"
#include "util.h"

typedef enum {
	PPPOAT_IF_TUN,
	PPPOAT_IF_TAP,
} tun_type_t;

struct tun_ctx {
	tun_type_t tc_type;
	int        tc_fd;
	int        tc_rd;
	int        tc_wr;
	char       tc_name[8];
	pthread_t  tc_thread;
	sem_t      tc_stop;
};

static const char *tun_path = "/dev/net/tun";

static int if_module_tun_init_common(struct pppoat_conf  *conf,
				     void               **userdata,
				     tun_type_t           type)
{
	struct tun_ctx *ctx;
	struct ifreq    ifr;
	int             rc;

	ctx = pppoat_alloc(sizeof(*ctx));
	PPPOAT_ASSERT(ctx != NULL);

	rc = sem_init(&ctx->tc_stop, 0, 0);
	PPPOAT_ASSERT(rc == 0);
	ctx->tc_type = type;

	ctx->tc_fd = open(tun_path, O_RDWR);
	PPPOAT_ASSERT(ctx->tc_fd >= 0);
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = type == PPPOAT_IF_TUN ? IFF_TUN : IFF_TAP;
	rc = ioctl(ctx->tc_fd, TUNSETIFF, (void *)&ifr);
	PPPOAT_ASSERT(rc >= 0);
	PPPOAT_ASSERT(strlen(ifr.ifr_name) < sizeof(ctx->tc_name));
	strcpy(ctx->tc_name, ifr.ifr_name);

	pppoat_debug("tun/tap", "Created interface %s", ctx->tc_name);
	*userdata = ctx;

	return 0;
}

static int if_module_tun_init(struct pppoat_conf *conf, void **userdata)
{
	return if_module_tun_init_common(conf, userdata, PPPOAT_IF_TUN);
}

/* this is the only TAP specific function in op vector */
static int if_module_tap_init(struct pppoat_conf *conf, void **userdata)
{
	return if_module_tun_init_common(conf, userdata, PPPOAT_IF_TAP);
}

static void if_module_tun_fini(void *userdata)
{
	struct tun_ctx *ctx = userdata;

	sem_destroy(&ctx->tc_stop);
	close(ctx->tc_fd);
	pppoat_free(ctx);
}

static void *tun_thread(void *userdata)
{
	struct tun_ctx *ctx = userdata;
	fd_set          rfds;
	int             max;
	int             rc;

	while (sem_trywait(&ctx->tc_stop) != 0) {
		FD_ZERO(&rfds);
		FD_SET(ctx->tc_rd, &rfds);
		FD_SET(ctx->tc_fd, &rfds);
		max = pppoat_max(ctx->tc_rd, ctx->tc_fd);
		rc  = pppoat_util_select(max, &rfds, NULL);
		PPPOAT_ASSERT(rc >= 0);

		if (FD_ISSET(ctx->tc_rd, &rfds)) {
			rc = pppoat_util_write_fd(ctx->tc_fd, ctx->tc_rd);
			PPPOAT_ASSERT(rc == 0);
		}
		if (FD_ISSET(ctx->tc_fd, &rfds)) {
			rc = pppoat_util_write_fd(ctx->tc_wr, ctx->tc_fd);
			PPPOAT_ASSERT(rc == 0);
		}
	}
	return NULL;
}

static int if_module_tun_run(int rd, int wr, void *userdata)
{
	struct tun_ctx *ctx = userdata;
	int             rc;

	ctx->tc_rd = dup(rd);
	ctx->tc_wr = dup(wr);
	PPPOAT_ASSERT(ctx->tc_rd != -1);
	PPPOAT_ASSERT(ctx->tc_wr != -1);
	rc = pthread_create(&ctx->tc_thread, NULL, &tun_thread, userdata);
	PPPOAT_ASSERT(rc == 0);

	return 0;
}

static int if_module_tun_stop(void *userdata)
{
	struct tun_ctx *ctx = userdata;
	int             rc;

	sem_post(&ctx->tc_stop);
	rc = pthread_join(ctx->tc_thread, NULL);
	PPPOAT_ASSERT(rc == 0);
	close(ctx->tc_rd);
	close(ctx->tc_wr);

	return 0;
}

const struct pppoat_if_module pppoat_if_module_tun = {
	.im_name  = "tun",
	.im_descr = "Using TUN/TAP driver",
	.im_init  = &if_module_tun_init,
	.im_fini  = &if_module_tun_fini,
	.im_run   = &if_module_tun_run,
	.im_stop  = &if_module_tun_stop,
};

const struct pppoat_if_module pppoat_if_module_tap = {
	.im_name  = "tap",
	.im_descr = "Using TUN/TAP driver",
	.im_init  = &if_module_tap_init,
	.im_fini  = &if_module_tun_fini,
	.im_run   = &if_module_tun_run,
	.im_stop  = &if_module_tun_stop,
};
