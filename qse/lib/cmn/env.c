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
#include <qse/cmn/str.h>
#include "mem.h"

#define STRSIZE 4096
#define ARRSIZE 128

QSE_IMPLEMENT_COMMON_FUNCTIONS(env)

static int load_curenv (qse_env_t* env);

qse_env_t* qse_env_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, int fromcurenv)
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

	if (qse_env_init (env, mmgr, fromcurenv) == QSE_NULL)
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

qse_env_t* qse_env_init (qse_env_t* env, qse_mmgr_t* mmgr, int fromcurenv)
{
	QSE_MEMSET (env, 0, QSE_SIZEOF(*env));
	env->mmgr = mmgr;

	if (fromcurenv && load_curenv (env) <= -1) return QSE_NULL;
	return env;
}

void qse_env_fini (qse_env_t* env)
{
	if (env->arr.ptr) QSE_MMGR_FREE (env->mmgr, env->arr.ptr);
	if (env->str.ptr) QSE_MMGR_FREE (env->mmgr, env->str.ptr);
}

void qse_env_clear (qse_env_t* env)
{
	if (env->str.ptr) 
	{
		env->str.ptr[0] = QSE_T('\0');
		env->str.len = 0;
	}
	if (env->arr.ptr) 
	{
		env->arr.ptr = QSE_NULL;
		env->arr.len = 0;
	}
}

static int expandarr (qse_env_t* env)
{
	qse_char_t** tmp;
	qse_size_t ncapa = env->arr.capa + ARRSIZE;

	tmp = (qse_char_t**) QSE_MMGR_REALLOC (
		env->mmgr, env->arr.ptr,
		QSE_SIZEOF(qse_char_t*) * (ncapa + 1));
	if (tmp == QSE_NULL) return -1;

	env->arr.ptr = tmp;
	env->arr.capa = ncapa;
	return 0;
}

static int expandstr (qse_env_t* env, qse_size_t inc)
{
	qse_char_t* tmp;
	qse_size_t ncapa = env->str.capa;

	ncapa = (inc > STRSIZE)? (ncapa + inc): (ncapa + STRSIZE);

	tmp = (qse_char_t*) QSE_MMGR_REALLOC (
		env->mmgr, env->str.ptr, 
		QSE_SIZEOF(qse_char_t) * (ncapa + 1));
	if (tmp == QSE_NULL) return -1;

	env->str.ptr = tmp;
	env->str.capa = ncapa;
	return 0;
}

int qse_env_insert (
	qse_env_t* env, const qse_char_t* name, const qse_char_t* value)
{
	qse_size_t nl, vl, tl;

	nl = qse_strlen (name);
	vl = qse_strlen (value);

	if (env->arr.len >= env->arr.capa &&
	    expandarr(env) <= -1) return -1;

	tl = nl + 1 + vl + 1; /* name = value '\0' */
	if (env->str.len + tl > env->str.capa &&
	    expandstr (env, tl) <= -1) return -1;

	env->arr.ptr[env->arr.len++] = &env->str.ptr[env->str.len];
	env->arr.ptr[env->arr.len] = QSE_NULL;

	env->str.len += qse_strcpy (&env->str.ptr[env->str.len], name);
	env->str.ptr[env->str.len++] = QSE_T('=');
	env->str.len += qse_strcpy (&env->str.ptr[env->str.len], value);
	env->str.ptr[++env->str.len] = QSE_T('\0'); 

	return -1;
}

int qse_env_delete (qse_env_t* env, const qse_char_t* name)
{
	const qse_char_t* p = env->str.ptr;
	qse_size_t i;

	for (i = 0; i < env->arr.len; i++)
	{
		const qse_char_t* eq;
		const qse_char_t* vp;

		vp = env->arr.ptr[i];

		eq = qse_strbeg (vp, name);
		if (eq && *eq == QSE_T('='))
		{
#if 0
			/* bingo */
			len = qse_strlen (vp) + 1;
			QSE_MEMCPY (vp, vp + len, ... );
#endif
		}
	}

	return 0;
}

static int add_envstr (qse_env_t* env, const qse_char_t* nv)
{
	qse_size_t tl;
	
	if (env->arr.len >= env->arr.capa &&
	    expandarr(env) <= -1) return -1;

	tl = qse_strlen(nv) + 1;
	if (env->str.len + tl > env->str.capa &&
	    expandstr (env, tl) <= -1) return -1;

	env->arr.ptr[env->arr.len++] = &env->str.ptr[env->str.len];
	env->arr.ptr[env->arr.len] = QSE_NULL;

	env->str.len += qse_strcpy (&env->str.ptr[env->str.len], nv);
	env->str.ptr[++env->str.len] = QSE_T('\0'); 
	return 0;
}

static int load_curenv (qse_env_t* env)
{
#if defined(_WIN32)
	qse_char_t* envstr;
	int ret = 0;

	envstr = GetEnvironmentStrings ();
	if (envstr == QSE_NULL) return -1;

	while (*envstr != QSE_T('\0'))
	{
		if (qse_env_addraw (env, envstr) <= -1) 
		{
			ret = -1;
			goto done;
		}
		envstr += qse_strlen(lpszVariable) + 1;
	}		

done:
	FreeEnvironmentStrings (envstr);
	return ret;

#elif defined(__OS2__)
	/* TODO: */
	return -1;
#elif defined(__DOS__)
	/* TODO: */
	return -1;
#else
	extern char** environ;
	char** p = environ;

#if defined(QSE_CHAR_IS_MCHAR)
	while (*p)
	{
		if (qse_env_addraw (env, *p) <= -1) return -1;
		p++;	
	}
				
#else
	while (*p)
	{
		int n;
		qse_char_t* x;

		x = qse_mbstowcsdup (*p, env->mmgr);
		if (x == QSE_NULL) return -1;

		n = add_envstr (env, x);
		QSE_MMGR_FREE (env->mmgr, x);

		if (n <= -1) return -1;

		p++;	
	}
#endif

	return 0;
#endif
}

