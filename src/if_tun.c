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
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include "trace.h"
#include "if_tun.h"
#include "if.h"
#include "log.h"
#include "memory.h"

typedef enum {
	PPPOAT_IF_TUN,
	PPPOAT_IF_TAP,
} tun_type_t;

struct tun_ctx {
	tun_type_t tc_type;
	int        tc_fd;
	char       tc_name[8];
};

static const char *tun_path = "/dev/net/tun";

static int if_module_tun_init_common(int          argc,
				     char       **argv,
				     void       **userdata,
				     tun_type_t   type)
{
	(void)tun_path;
	return -ENOSYS;
}

static int if_module_tun_init(int argc, char **argv, void **userdata)
{
	return if_module_tun_init_common(argc, argv, userdata, PPPOAT_IF_TUN);
}

/* this is the only TAP specific function in op vector */
static int if_module_tap_init(int argc, char **argv, void **userdata)
{
	return if_module_tun_init_common(argc, argv, userdata, PPPOAT_IF_TAP);
}

static void if_module_tun_fini(void *userdata)
{
}

static int if_module_tun_run(int rd, int wr, void *userdata)
{
	return -ENOSYS;
}

static int if_module_tun_stop(void *userdata)
{
	return -ENOSYS;
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
