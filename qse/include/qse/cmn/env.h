/*
 * $Id: pio.h 455 2011-05-09 16:11:13Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_CMN_ENV_H_
#define _QSE_CMN_ENV_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_env_t qse_env_t;

struct qse_env_t
{
	QSE_DEFINE_COMMON_FIELDS(pio)

	struct
	{
		qse_size_t  capa;
		qse_size_t  len;
		void*       ptr;
	} buf;

	struct
	{
		qse_size_t  capa;
		qse_size_t  len;
		void**      ptr;
	} arr;
};


#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS(env)

qse_env_t* qse_env_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize
);

void qse_env_close (
	qse_env_t* env
);

qse_env_t* qse_env_init (
	qse_env_t*  env,
	qse_mmgr_t* mmgr
);

void qse_env_fini (
	qse_env_t* env
);

/*
void* qse_env_getstring ();
void* qse_env_getarray ();
*/

int qse_env_add (
	qse_env_t*  env,
	const void* name,
	const void* value
);

#ifdef __cplusplus
}
#endif

#endif
