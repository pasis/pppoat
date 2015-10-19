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

	fprintf(f, "Usage: %s [options]\n\n", name);
	fprintf(f, "Options:\n"
		   "  --help    Print this help\n"
		   "  --list    Print list of available modules\n"
		   "  -s        Server mode\n");
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
	pppoat_node_type_t          type = PPPOAT_NODE_SLAVE;
	const char                 *pppd;
	const char                 *ip;
	void                       *userdata;
	pid_t                       pid;
	int                         rd[2];
	int                         wr[2];
	int                         rc;

	pppoat_log_init(PPPOAT_DEBUG);

	if (argc > 1 && strcmp(argv[1], "--help") == 0) {
		help_print(stdout, argv[0]);
		exit(0);
	}
	if (argc > 1 && strcmp(argv[1], "--list") == 0) {
		module_list_print(stdout);
		exit(0);
	}
	if (argc > 1 && strcmp(argv[1], "-s") == 0) {
		type = PPPOAT_NODE_MASTER;
	}
	pppd = pppd_find();
	pppoat_debug("main", "pppd path: %s", pppd == NULL ? "NULL" : pppd);
	m = module_find("udp");
	PPPOAT_ASSERT(m != NULL);

	rc = m->m_init(argc, argv, &userdata);
	PPPOAT_ASSERT_INFO(rc == 0, "rc=%d", rc);
	ip = type == PPPOAT_NODE_MASTER ? "10.0.0.1:10.0.0.2" : NULL;

	/* create pipes for communication with pppd */
	rc = pipe(rd);
	PPPOAT_ASSERT(rc == 0);
	rc = pipe(wr);
	PPPOAT_ASSERT(rc == 0);

	/* exec pppd */
	pid = fork();
	PPPOAT_ASSERT(pid >= 0);
	if (pid == 0) {
		rc = dup2(rd[1], 1);
		PPPOAT_ASSERT(rc >= 0);
		rc = dup2(wr[0], 0);
		PPPOAT_ASSERT(rc >= 0);
		close(rd[0]);
		close(rd[1]);
		close(wr[0]);
		close(wr[1]);
		pppoat_debug("main", "%s nodetach noauth notty passive %s",
			     pppd, ip == NULL ? "" : ip);
		execl(pppd, pppd, "nodetach", "noauth",
		      "notty", "passive", ip, NULL);
		pppoat_error("main", "execl() should never return");
		exit(1);
	}

	close(rd[1]);
	close(wr[0]);

	/* run appropriate module's function */
	rc = m->m_run(rd[0], wr[1], 0 /* XXX */, userdata);
	pppoat_error("main", "rc=%d", rc);

	m->m_fini(userdata);
	pppoat_log_fini();

	return 0;
}
