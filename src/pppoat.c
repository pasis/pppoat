/* pppoat.c
 * PPP over Any Transport -- Main routines
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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "pppoat.h"
#include "log.h"
#include "util.h"

#include "modules/udp.h"
#include "modules/xmpp.h"

static const char *pppd_paths[] = {
	"/sbin/pppd",
	"/usr/sbin/pppd",
	"/usr/local/sbin/pppd",
	"/usr/bin/pppd",
	"/usr/local/bin/pppd",
};

static const struct pppoat_module *module_tbl[] = 
{
	&pppoat_module_udp,
	&pppoat_module_xmpp,
};

static void help_print(FILE *f, char *name)
{
#ifdef PACKAGE_STRING
	fprintf(f, PACKAGE_STRING "\n\n");
#endif /* PACKAGE_STRING */

	fprintf(f, "Usage: %s [options]\n", name);
}

static const struct pppoat_module *module_find(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(module_tbl); ++i) {
		if (strcmp(module_tbl[i]->m_name, name) == 0)
			break;
	}
	return i < ARRAY_SIZE(module_tbl) ? module_tbl[i] : NULL;
}

static void module_list_print(FILE *f)
{
	int i;

	fprintf(f, "List of modules:\n");
	for (i = 0; i < ARRAY_SIZE(module_tbl); ++i) {
		fprintf(f, "  %s   %s\n", module_tbl[i]->m_name,
					  module_tbl[i]->m_descr);
	}
}

static const char *pppd_find(void)
{
	int rc;
	int i;

	for (i = 0; i < ARRAY_SIZE(pppd_paths); ++i) {
		rc = access(pppd_paths[i], X_OK);
		if (rc == 0) {
			break;
		} else if (errno != ENOENT) {
			pppoat_info("%s exists, but access(2) returned"
				    "errno=%d", pppd_paths[i], errno);
		}
	}
	return i < ARRAY_SIZE(pppd_paths) ? pppd_paths[i] : NULL;
}

int main(int argc, char **argv)
{
	const struct pppoat_module *m;
	const char                 *pppd;
	void                       *userdata;
	int                         rc;

	pppoat_log_init(PPPOAT_DEBUG);

	/* XXX */
	help_print(stdout, argv[0]);
	module_list_print(stdout);
	pppd = pppd_find();
	if (pppd != NULL) {
		printf("found pppd: %s\n", pppd);
	}
	m = module_find("udp");
	PPPOAT_ASSERT(m != NULL);

	rc = m->m_init(argc, argv, &userdata);
	PPPOAT_ASSERT(rc == 0);
	rc = m->m_run(0, 1, 0, userdata);
	pppoat_error("main", "rc=%d", rc);
	m->m_fini(userdata);

#if 0 /* XXX from old version */
	/* create pipes for communication with pppd */
	if (pipe(pd_rd) < 0)
		err_exit("pipe");
	if (pipe(pd_wr) < 0)
		err_exit("pipe");

	/* exec pppd */
	if ((pid = fork()) < 0)
		err_exit("fork");
	if (!pid) {
		if (dup2(pd_rd[1], 1) < 0)
			err_exit("dup2");
		if (dup2(pd_wr[0], 0) < 0)
			err_exit("dup2");
		close(pd_rd[0]);
		close(pd_rd[1]);
		close(pd_wr[0]);
		close(pd_wr[1]);
		execl(pppd, pppd, "nodetach", "noauth", "notty", "passive", ip, NULL);
		err_exit("execl");
	}

	close(pd_rd[1]);
	close(pd_wr[0]);

	/* run appropriate module's function */
	return mod_tbl[mod_idx].func(mod_argc, mod_argv, pd_rd[0], pd_wr[1]);
#endif

	pppoat_log_fini();

	return 0;
}
