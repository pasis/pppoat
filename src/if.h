/* if.h
 * PPP over Any Transport -- Network interface modules
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

#ifndef __PPPOAT_IF_H__
#define __PPPOAT_IF_H__

/*
 * FIXME: documentation
 * XXX im_run() isn't blocking.
 * XXX describe that module has to dup(2) file descriptors if it uses
 *     them in the same process. pppoat closes rd,wr when im_run()
 *     returns.
 */

struct pppoat_if_module {
	const char *im_name;
	const char *im_descr;
	int       (*im_init)(struct pppoat_conf *conf, void **userdata);
	void      (*im_fini)(void *userdata);
	int       (*im_run)(int rd, int wr, void *userdata);
	int       (*im_stop)(void *userdata);
};

#endif /* __PPPOAT_IF_H__ */
