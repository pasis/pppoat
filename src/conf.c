/* conf.c
 * PPP over Any Transport -- Configuration
 *
 * Copyright (C) 2012-2017 Dmitry Podgorny <pasis.ua@gmail.com>
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

#include <string.h>	/* strcmp */
#include <unistd.h>	/* getopt */
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>	/* getopt_long */
#endif /* HAVE_GETOPT_LONG */

#include "trace.h"
#include "conf.h"
#include "memory.h"

#define CONF_KEYS_MAX 10

int pppoat_conf_init(struct pppoat_conf *conf)
{
	conf->cfg_keys = pppoat_calloc(CONF_KEYS_MAX, sizeof(*conf->cfg_keys));
	conf->cfg_vals = pppoat_calloc(CONF_KEYS_MAX, sizeof(*conf->cfg_vals));

	return 0;
}

void pppoat_conf_fini(struct pppoat_conf *conf)
{
	int i;

	for (i = 0; i < CONF_KEYS_MAX; ++i) {
		pppoat_free(conf->cfg_keys[i]);
		pppoat_free(conf->cfg_vals[i]);
	}
	pppoat_free(conf->cfg_keys);
	pppoat_free(conf->cfg_vals);
}

int pppoat_conf_insert(struct pppoat_conf *conf, const char *key,
		       const char *obj)
{
	int i;

	PPPOAT_ASSERT(pppoat_conf_get(conf, key) == NULL);

	for (i = 0; i < CONF_KEYS_MAX; ++i)
		if (conf->cfg_keys[i] == NULL) {
			conf->cfg_keys[i] = pppoat_strdup(key);
			conf->cfg_vals[i] = pppoat_strdup(obj);
			break;
		}
	return i == CONF_KEYS_MAX ? -ENOMEM : 0;
}

int pppoat_conf_update(struct pppoat_conf *conf, const char *key,
		       const char *obj)
{
	/* TODO implement replace */
	return pppoat_conf_insert(conf, key, obj);
}

void pppoat_conf_remove(struct pppoat_conf *conf, const char *key)
{
	int i;

	for (i = 0; i < CONF_KEYS_MAX; ++i)
		if (strcmp(key, conf->cfg_keys[i]) == 0) {
			pppoat_free(conf->cfg_keys[i]);
			pppoat_free(conf->cfg_vals[i]);
			conf->cfg_keys[i] = NULL;
			conf->cfg_vals[i] = NULL;
		}
}

const char *pppoat_conf_get(const struct pppoat_conf *conf, const char *key)
{
	int i;

	for (i = 0; i < CONF_KEYS_MAX; ++i)
		if (conf->cfg_keys[i] != NULL &&
		    strcmp(conf->cfg_keys[i], key) == 0)
			break;
	return i < CONF_KEYS_MAX ? conf->cfg_vals[i] : NULL;
}

bool pppoat_conf_obj_is_true(const char *obj)
{
	return obj != NULL && (strcmp(obj, "true") == 0 ||
			       strcmp(obj, "1")    == 0);
}

static void _conf_dump(const struct pppoat_conf *conf)
{
	int i;

	pppoat_debug("conf", "Content:");
	for (i = 0; i < CONF_KEYS_MAX; ++i)
		if (conf->cfg_keys[i] != NULL)
			pppoat_debug("conf", "  %s = %s", conf->cfg_keys[i],
							  conf->cfg_vals[i]);
}

int pppoat_conf_args_parse(struct pppoat_conf *conf, int argc, char **argv)
{
	size_t  len;
	char   *key;
	char   *val;
	int     opt;
	int     i;

	/*
	 * TODO Create array of supported options with descriptions.
	 * Construct usage help and longopts on fly.
	 */
	static const struct option longopts[] = {
		{ "dest",   required_argument, NULL, 'd' },
		{ "help",   no_argument,       NULL, 'h' },
		{ "if",     required_argument, NULL, 'i' },
		{ "list",   no_argument,       NULL, 'l' },
		{ "module", required_argument, NULL, 'm' },
		{ "server", no_argument,       NULL, 'S' },
		{ "src",    required_argument, NULL, 's' },
		{ NULL, 0, NULL, 0 }
	};
	static const char *optstring = "d:hilSs:m:";

	while (1) {
#ifdef HAVE_GETOPT_LONG
		opt = getopt_long(argc, argv, optstring, longopts, NULL);
#else
		opt = getopt(argc, argv, optstring);
#endif /* HAVE_GETOPT_LONG */
		if (opt == -1)
			break;

		switch (opt) {
		case 'd':
			pppoat_conf_update(conf, "destination", optarg);
			break;
		case 'h':
			pppoat_conf_update(conf, "help", "true");
			break;
		case 'i':
			pppoat_conf_update(conf, "if", optarg);
			break;
		case 'l':
			pppoat_conf_update(conf, "list", "true");
			break;
		case 'm':
			pppoat_conf_update(conf, "module", optarg);
			break;
		case 'S':
			pppoat_conf_update(conf, "server", "true");
			break;
		case 's':
			pppoat_conf_update(conf, "source", optarg);
			break;
		default:
			;
		}
	}
	for (i = optind; i < argc; ++i) {
		len = strcspn(argv[i], "=");
		key = pppoat_alloc(len + 1);
		PPPOAT_ASSERT(key != NULL);
		memcpy(key, argv[i], len);
		key[len] = '\0';
		val = argv[i][len] == '=' ? &argv[i][len + 1] : "true";
		pppoat_conf_update(conf, key, val);
		pppoat_free(key);
	}
	/* XXX debug */
	_conf_dump(conf);

	return 0;
}
