/* config.h
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

#ifndef __PPPOAT_CONFIG_H__
#define __PPPOAT_CONFIG_H__

/*
 * XXX Current interface is temporary. Conf should provide thread-safe interface
 * for adding/replacing objects in run-time. Get-like funcitons must not return
 * pointers to internal stored objects, because they can be destroyed
 * concurrently (or implement referernce counting).
 */

struct pppoat_conf {
	/* TODO hash table */
	char **cfg_keys;
	char **cfg_vals;
};

int pppoat_conf_init(struct pppoat_conf *conf);
void pppoat_conf_fini(struct pppoat_conf *conf);

int pppoat_conf_insert(struct pppoat_conf *conf, const char *key,
		       const char *obj);
int pppoat_conf_update(struct pppoat_conf *conf, const char *key,
		       const char *obj);
void pppoat_conf_remove(struct pppoat_conf *conf, const char *key);
const char *pppoat_conf_get(const struct pppoat_conf *conf, const char *key);
bool pppoat_conf_obj_is_true(const char *obj);

/* interface for reading cfg file (ini) */

int pppoat_conf_args_parse(struct pppoat_conf *conf, int argc, char **argv);

#endif /* __PPPOAT_CONFIG_H__ */
