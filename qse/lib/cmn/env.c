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


#include <qse/cmn/env.h>
#include "mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS(env)

qse_env_t* qse_env_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_env_t* env;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	env = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_env_t) + xtnsize);
	if (env == QSE_NULL) return QSE_NULL;

	if (qse_env_init (env, mmgr) == QSE_NULL)
	{
		QSE_MMGR_FREE (mmgr, env);
		return QSE_NULL;
	}

	return env;
}

void qse_env_close (qse_env_t* env)
{
	qse_env_fini (env);
	QSE_MMGR_FREE (env->mmgr, env);
}

qse_env_t* qse_env_init (qse_env_t* env, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (env, 0, QSE_SIZEOF(*env));
	return env;
}

void qse_env_fini (qse_env_t* env)
{
	if (env->arr.ptr) QSE_MMGR_FREE (env->mmgr, env->arr.ptr);
	if (env->buf.ptr) QSE_MMGR_FREE (env->mmgr, env->buf.ptr);
}


static int expand_buffer (qse_env_t* env)
{
	if (env->buf.ptr == QSE_NULL)
	{
	}
	return -1;
}

static int expand_array (qse_env_t* env)
{
	if (env->arr.ptr == QSE_NULL)
	{
	}
	return -1;
}

int qse_env_add (qse_env_t* env, const void* name, const void* value)
{
	return -1;
}
