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

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "pppoat.h"
#include "if.h"
#include "log.h"
#include "util.h"

#include "if_pppd.h"
#include "if_stdio.h"
#include "if_tun.h"
#include "modules/udp.h"
#include "modules/xmpp.h"

static const struct pppoat_module *module_tbl[] =
{
	&pppoat_module_udp,
	&pppoat_module_xmpp,
};

static const struct pppoat_if_module *if_module_tbl[] =
{
	&pppoat_if_module_pppd,
	&pppoat_if_module_tun,
	&pppoat_if_module_tap,
	&pppoat_if_module_stdio,
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

int main(int argc, char **argv)
{
	const struct pppoat_module    *m;
	const struct pppoat_if_module *im;
	void                          *m_data;
	void                          *im_data;
	int                            rd[2];
	int                            wr[2];
	int                            rc;

	pppoat_log_init(PPPOAT_DEBUG);

	if (argc > 1 && strcmp(argv[1], "--help") == 0) {
		help_print(stdout, argv[0]);
		exit(0);
	}
	if (argc > 1 && strcmp(argv[1], "--list") == 0) {
		module_list_print(stdout);
		exit(0);
	}

	im = if_module_tbl[0];
	PPPOAT_ASSERT(im != NULL);
	m = module_find("udp");
	PPPOAT_ASSERT(m != NULL);

	/* init modules */
	rc = im->im_init(argc, argv, &im_data);
	PPPOAT_ASSERT_INFO(rc == 0, "rc=%d", rc);
	rc = m->m_init(argc, argv, &m_data);
	PPPOAT_ASSERT_INFO(rc == 0, "rc=%d", rc);

	/* create pipes for communication with pppd */
	rc = pipe(rd);
	PPPOAT_ASSERT(rc == 0);
	rc = pipe(wr);
	PPPOAT_ASSERT(rc == 0);

	/* exec pppd */
	rc = im->im_run(wr[0], rd[1], im_data);
	PPPOAT_ASSERT_INFO(rc == 0, "rc=%d", rc);
	close(rd[1]);
	close(wr[0]);

	/* run appropriate module's function */
	rc = m->m_run(rd[0], wr[1], 0 /* XXX */, m_data);
	pppoat_error("main", "rc=%d", rc);

	/* finalisation */
	im->im_stop(im_data);
	im->im_fini(im_data);
	m->m_fini(m_data);
	close(rd[0]);
	close(wr[1]);

	pppoat_log_fini();

	return 0;
}
