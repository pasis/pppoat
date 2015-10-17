/* pppoat.h
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

#ifndef __PPPOAT_PPPOAT_H__
#define __PPPOAT_PPPOAT_H__

/**
 * FIXME Documentation
 * XXX pass main config to init()
 * XXX add some get() that returns MASTER/SLAVE, ip, etc
 */
struct pppoat_module {
	const char *m_name;
	const char *m_descr;
	int       (*m_init)(int argc, char **argv, void **userdata);
	void      (*m_fini)(void *userdata);
	int       (*m_run)(int rd, int wr, int ctrl, void *userdata);
};

#endif /* __PPPOAT_PPPOAT_H__ */
