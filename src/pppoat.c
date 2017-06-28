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
#include "conf.h"
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

/* TODO Construct help message from the conf array. */
static void help_print(FILE *f, char *name)
{
#ifdef PACKAGE_STRING
	fprintf(f, PACKAGE_STRING "\n\n");
#endif /* PACKAGE_STRING */

	fprintf(f, "Usage: %s [options]\n\n", name);
	fprintf(f, "Options:\n"
		   "  --dest=<ip> (-d)     Destination IP for the tunnel\n"
		   "  --help (-h)          Print this help\n"
		   "  --if=<name> (-i)     Interface module name\n"
		   "  --list (-l)          Print list of available modules\n"
		   "  --module=<name> (-m) Transport module name\n"
		   "  --server (-S)        Server mode\n"
		   "  --src=<ip> (-s)      Source IP for the tunnel\n");
}

static const struct pppoat_module *module_find(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(module_tbl); ++i)
		if (strcmp(module_tbl[i]->m_name, name) == 0)
			break;
	return i < ARRAY_SIZE(module_tbl) ? module_tbl[i] : NULL;
}

static const struct pppoat_if_module *if_module_find(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(if_module_tbl); ++i)
		if (strcmp(if_module_tbl[i]->im_name, name) == 0)
			break;
	return i < ARRAY_SIZE(if_module_tbl) ? if_module_tbl[i] : NULL;
}

static void module_list_print(FILE *f)
{
	int i;

	fprintf(f, "List of interface modules:\n");
	for (i = 0; i < ARRAY_SIZE(if_module_tbl); ++i)
		fprintf(f, "  %s   %s\n", if_module_tbl[i]->im_name,
					  if_module_tbl[i]->im_descr);
	fprintf(f, "\n");
	fprintf(f, "List of transport modules:\n");
	for (i = 0; i < ARRAY_SIZE(module_tbl); ++i)
		fprintf(f, "  %s   %s\n", module_tbl[i]->m_name,
					  module_tbl[i]->m_descr);
}

int main(int argc, char **argv)
{
	const struct pppoat_module    *m;
	const struct pppoat_if_module *im;
	struct pppoat_conf             conf;
	const char                    *if_name;
	void                          *m_data;
	void                          *im_data;
	bool                           present;
	int                            rd[2];
	int                            wr[2];
	int                            rc;

	pppoat_log_init(PPPOAT_DEBUG);
	rc = pppoat_conf_init(&conf);
	PPPOAT_ASSERT(rc == 0);
	rc = pppoat_conf_args_parse(&conf, argc, argv);
	PPPOAT_ASSERT(rc == 0);

	if (pppoat_conf_obj_is_true(pppoat_conf_get(&conf, "help"))) {
		help_print(stdout, argv[0]);
		exit(0);
	}
	if (pppoat_conf_obj_is_true(pppoat_conf_get(&conf, "list"))) {
		module_list_print(stdout);
		exit(0);
	}

	/* XXX Check mandatory options. Replace with better solution. */
	present = pppoat_conf_get(&conf, "module") != NULL;
	PPPOAT_ASSERT_INFO(present, "Mandatory option is missed");

	if_name = pppoat_conf_get(&conf, "if");
	im = if_name == NULL ? if_module_tbl[0] : if_module_find(if_name);
	PPPOAT_ASSERT(im != NULL);
	m = module_find(pppoat_conf_get(&conf, "module"));
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

	pppoat_conf_fini(&conf);
	pppoat_log_fini();

	return 0;
}
