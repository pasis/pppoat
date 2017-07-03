/* if_pppd.c
 * PPP over Any Transport -- pppd network interface module
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

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "conf.h"
#include "if_pppd.h"
#include "if.h"
#include "log.h"
#include "memory.h"
#include "util.h"

struct pppd_ctx {
	const char *pc_pppd;
	const char *pc_ip;
	pid_t       pc_pid;
};

static const char *pppd_paths[] = {
	"/sbin/pppd",
	"/usr/sbin/pppd",
	"/usr/local/sbin/pppd",
	"/usr/bin/pppd",
	"/usr/local/bin/pppd",
};

static const char *pppd_find(void)
{
	int rc;
	int i;

	for (i = 0; i < ARRAY_SIZE(pppd_paths); ++i) {
		rc = access(pppd_paths[i], X_OK);
		if (rc == 0) {
			break;
		} else if (errno != ENOENT) {
			pppoat_info("pppd", "%s exists, but access(2) returned"
				    "errno=%d", pppd_paths[i], errno);
		}
	}
	return i < ARRAY_SIZE(pppd_paths) ? pppd_paths[i] : NULL;
}

static int if_module_pppd_init(struct pppoat_conf *conf, void **userdata)
{
	struct pppd_ctx *ctx;
	const char      *obj;
	int              rc;

	ctx = pppoat_alloc(sizeof(*ctx));
	rc  = ctx == NULL ? P_ERR(-ENOMEM) : 0;
	if (rc == 0) {
		ctx->pc_ip   = NULL;
		ctx->pc_pppd = pppd_find();
		rc = ctx->pc_pppd == NULL ? P_ERR(-ENOENT) : 0;
		if (rc != 0)
			pppoat_free(ctx);
	}

	if (rc == 0) {
		obj = pppoat_conf_get(conf, "server");
		/* XXX make more flexible ip configuration */
		if (obj != NULL && pppoat_conf_obj_is_true(obj))
			ctx->pc_ip = "10.0.0.1:10.0.0.2";
		*userdata = ctx;
	}
	return rc;
}

static void if_module_pppd_fini(void *userdata)
{
	pppoat_free(userdata);
}

static int if_module_pppd_run(int rd, int wr, void *userdata)
{
	struct pppd_ctx *ctx  = userdata;
	const char      *pppd = ctx->pc_pppd;
	const char      *ip   = ctx->pc_ip;
	pid_t            pid;
	int              rc;

	pid = fork();
	PPPOAT_ASSERT(pid >= 0);
	if (pid == 0) {
		rc = dup2(rd, 0);
		PPPOAT_ASSERT(rc >= 0);
		rc = dup2(wr, 1);
		PPPOAT_ASSERT(rc >= 0);
		close(rd);
		close(wr);
		/* XXX opposite sides of the pipes are not closed */
		pppoat_debug("pppd", "%s nodetach noauth notty passive %s",
			     pppd, ip == NULL ? "" : ip);
		execl(pppd, pppd, "nodetach", "noauth",
		      "notty", "passive", ip, NULL);
		pppoat_error("main", "execl() should never return");
		exit(1);
	}
	ctx->pc_pid = pid;

	return 0;
}

static int if_module_pppd_stop(void *userdata)
{
	struct pppd_ctx *ctx = userdata;
	pid_t            pid;
	int              rc;

	rc = kill(ctx->pc_pid, SIGTERM);
	PPPOAT_ASSERT(rc == 0);
	do {
		pid = waitpid(ctx->pc_pid, NULL, 0);
	} while (pid < 0 && errno == EINTR);
	PPPOAT_ASSERT(pid > 0);

	return 0;
}

const struct pppoat_if_module pppoat_if_module_pppd = {
	.im_name  = "ppp",
	.im_descr = "Using pppd",
	.im_init  = &if_module_pppd_init,
	.im_fini  = &if_module_pppd_fini,
	.im_run   = &if_module_pppd_run,
	.im_stop  = &if_module_pppd_stop,
};
