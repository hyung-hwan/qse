/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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
#include <qse/cmn/mbwc.h>
#include "mem.h"

#if defined(_WIN32)
#    include <windows.h>
#endif

#if defined(HAVE_CRT_EXTERNS_H)
	/* MacOSX/darwin. _NSGetEnviron() */
#	include <crt_externs.h>
#endif

#define STRSIZE 4096
#define ARRSIZE 128

static int load_curenv (qse_env_t* env);
static int insert_sys_wcs (qse_env_t* env, const qse_wchar_t* name);
static int insert_sys_mbs (qse_env_t* env, const qse_mchar_t* name);

qse_env_t* qse_env_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, int fromcurenv)
{
	qse_env_t* env;

	env = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_env_t) + xtnsize);
	if (env)
	{
		if (qse_env_init (env, mmgr, fromcurenv) <= -1)
		{
			QSE_MMGR_FREE (mmgr, env);
			return QSE_NULL;
		}
		else QSE_MEMSET (QSE_XTN(env), 0, xtnsize);
	}

	return env;
}

void qse_env_close (qse_env_t* env)
{
	qse_env_fini (env);
	QSE_MMGR_FREE (env->mmgr, env);
}

int qse_env_init (qse_env_t* env, qse_mmgr_t* mmgr, int fromcurenv)
{
	QSE_MEMSET (env, 0, QSE_SIZEOF(*env));
	env->mmgr = mmgr;

	if (fromcurenv && load_curenv (env) <= -1) return -1;
	return 0;
}

void qse_env_fini (qse_env_t* env)
{
	if (env->arr.ptr) QSE_MMGR_FREE (env->mmgr, env->arr.ptr);
	if (env->str.ptr) QSE_MMGR_FREE (env->mmgr, env->str.ptr);
}

qse_mmgr_t* qse_env_getmmgr (qse_env_t* env)
{
	return env->mmgr;
}

void* qse_env_getxtn (qse_env_t* env)
{
	return QSE_XTN (env);
}

void qse_env_clear (qse_env_t* env)
{
	if (env->str.ptr) 
	{
#if defined(QSE_ENV_CHAR_IS_WCHAR)
		env->str.ptr[0] = QSE_WT('\0');
#else
		env->str.ptr[0] = QSE_MT('\0');
#endif
		env->str.len = 0;
	}
	if (env->arr.ptr) 
	{
		env->arr.ptr[0] = QSE_NULL;
		env->arr.len = 0;
	}
}

const qse_env_char_t* qse_env_getstr (qse_env_t* env)
{
	if (env->str.ptr) return env->str.ptr;
	else
	{
		static qse_env_char_t empty[2] = { 0, 0 };
		return empty;
	}
}

qse_env_char_t** qse_env_getarr (qse_env_t* env)
{
	if (env->arr.ptr) return env->arr.ptr;
	else
	{
		static qse_env_char_t* empty[1] = { QSE_NULL };
		return empty;
	}
}

static int expandarr (qse_env_t* env)
{
	qse_env_char_t** tmp;
	qse_size_t ncapa = env->arr.capa + ARRSIZE;

	tmp = (qse_env_char_t**) QSE_MMGR_REALLOC (
		env->mmgr, env->arr.ptr,
		QSE_SIZEOF(qse_char_t*) * (ncapa + 1));
	if (tmp == QSE_NULL) return -1;

	env->arr.ptr = tmp;
	env->arr.capa = ncapa;

	return 0;
}

static int expandstr (qse_env_t* env, qse_size_t inc)
{
	qse_env_char_t* tmp;
	qse_size_t ncapa;

	ncapa = (inc > STRSIZE)? 
		(env->str.capa + inc): (env->str.capa + STRSIZE);

	tmp = (qse_env_char_t*) QSE_MMGR_REALLOC (
		env->mmgr, env->str.ptr, 
		QSE_SIZEOF(*tmp) * (ncapa + 1));
	if (tmp == QSE_NULL) return -1;

	if (tmp != env->str.ptr)
	{
		/* reallocation relocated the string buffer.
		 * the pointers in the pointer array have to be adjusted */
		qse_size_t i;
		for (i = 0; i < env->arr.len; i++)
		{
			qse_env_char_t* cur = env->arr.ptr[i];
			env->arr.ptr[i] = tmp + (cur - env->str.ptr);
		}
	}

	env->str.ptr = tmp;
	env->str.capa = ncapa;

	return 0;
}

#if defined(QSE_ENV_CHAR_IS_WCHAR)
static int insertw (qse_env_t* env, const qse_wchar_t* name, const qse_wchar_t* value[])
{
	qse_size_t nl, vl, tl, i;

	nl = qse_wcslen (name);
	for (i = 0, vl = 0; value[i]; i++) vl += qse_wcslen(value[i]);
	
	if (env->arr.len >= env->arr.capa &&
	    expandarr(env) <= -1) return -1;

	tl = nl + 1 + vl + 1; /* name = value '\0' */
	if (env->str.len + tl > env->str.capa &&
	    expandstr (env, tl) <= -1) return -1;

	env->arr.ptr[env->arr.len++] = &env->str.ptr[env->str.len];
	env->arr.ptr[env->arr.len] = QSE_NULL;

	env->str.len += qse_wcscpy (&env->str.ptr[env->str.len], name);
	env->str.ptr[env->str.len++] = QSE_WT('=');
	for (i = 0; value[i]; i++) 
		env->str.len += qse_wcscpy (&env->str.ptr[env->str.len], value[i]);
	env->str.ptr[++env->str.len] = QSE_WT('\0'); 

	return 0;
}

static int add_envstrw (qse_env_t* env, const qse_wchar_t* nv)
{
	qse_size_t tl;
	
	if (env->arr.len >= env->arr.capa &&
	    expandarr(env) <= -1) return -1;

	tl = qse_wcslen(nv) + 1;
	if (env->str.len + tl > env->str.capa &&
	    expandstr (env, tl) <= -1) return -1;

	env->arr.ptr[env->arr.len++] = &env->str.ptr[env->str.len];
	env->arr.ptr[env->arr.len] = QSE_NULL;

	env->str.len += qse_wcscpy (&env->str.ptr[env->str.len], nv);
	env->str.ptr[++env->str.len] = QSE_WT('\0'); 
	return 0;
}

static int deletew (qse_env_t* env, const qse_wchar_t* name)
{
	qse_size_t i;

	for (i = 0; i < env->arr.len; i++)
	{
		const qse_wchar_t* eq;
		qse_wchar_t* vp;

		vp = env->arr.ptr[i];

		eq = qse_wcsbeg (vp, name);
		if (eq && *eq == QSE_WT('='))
		{
			/* bingo */
			qse_size_t len, rem;

			len = qse_wcslen (vp) + 1;
			rem = env->str.len - (vp + len - env->str.ptr) + 1;
			QSE_MEMCPY (vp, vp + len, rem * QSE_SIZEOF(*vp));
			env->str.len -= len;

			env->arr.len--;
			for (; i < env->arr.len; i++)
				env->arr.ptr[i] = env->arr.ptr[i+1] - len;
			env->arr.ptr[i] = QSE_NULL;

			return 0;
		}
	}

	return -1;
}

#else
static int insertm (qse_env_t* env, const qse_mchar_t* name, const qse_mchar_t* value[])
{
	qse_size_t nl, vl, tl, i;

	nl = qse_mbslen (name);
	for (i = 0, vl = 0; value[i]; i++) vl += qse_mbslen(value[i]);

	if (env->arr.len >= env->arr.capa &&
	    expandarr(env) <= -1) return -1;

	tl = nl + 1 + vl + 1; /* name = value '\0' */
	if (env->str.len + tl > env->str.capa &&
	    expandstr (env, tl) <= -1) return -1;

	env->arr.ptr[env->arr.len++] = &env->str.ptr[env->str.len];
	env->arr.ptr[env->arr.len] = QSE_NULL;

	env->str.len += qse_mbscpy (&env->str.ptr[env->str.len], name);
	env->str.ptr[env->str.len++] = QSE_MT('=');
	for (i = 0; value[i]; i++)
		env->str.len += qse_mbscpy (&env->str.ptr[env->str.len], value[i]);
	env->str.ptr[++env->str.len] = QSE_MT('\0'); 

	return 0;
}

static int add_envstrm (qse_env_t* env, const qse_mchar_t* nv)
{
	qse_size_t tl;
	
	if (env->arr.len >= env->arr.capa &&
	    expandarr(env) <= -1) return -1;

	tl = qse_mbslen(nv) + 1;
	if (env->str.len + tl > env->str.capa &&
	    expandstr (env, tl) <= -1) return -1;

	env->arr.ptr[env->arr.len++] = &env->str.ptr[env->str.len];
	env->arr.ptr[env->arr.len] = QSE_NULL;

	env->str.len += qse_mbscpy (&env->str.ptr[env->str.len], nv);
	env->str.ptr[++env->str.len] = QSE_MT('\0'); 
	return 0;
}

static int deletem (qse_env_t* env, const qse_mchar_t* name)
{
	qse_size_t i;

	for (i = 0; i < env->arr.len; i++)
	{
		const qse_mchar_t* eq;
		qse_mchar_t* vp;

		vp = env->arr.ptr[i];

		eq = qse_mbsbeg (vp, name);
		if (eq && *eq == QSE_MT('='))
		{
			/* bingo */
			qse_size_t len, rem;

			len = qse_mbslen (vp) + 1;
			rem = env->str.len - (vp + len - env->str.ptr) + 1;
			QSE_MEMCPY (vp, vp + len, rem * QSE_SIZEOF(*vp));
			env->str.len -= len;

			env->arr.len--;
			for (; i < env->arr.len; i++)
				env->arr.ptr[i] = env->arr.ptr[i+1] - len;
			env->arr.ptr[i] = QSE_NULL;

			return 0;
		}
	}

	return -1;
}
#endif

static QSE_INLINE int insert_wcs (
	qse_env_t* env, const qse_wchar_t* name, const qse_wchar_t* value[])
{
#if defined(QSE_ENV_CHAR_IS_WCHAR)
	/* no conversion -> wchar */
	return insertw (env, name, value);
#else
	/* convert wchar to mchar */
	qse_mchar_t* namedup, * valuedup[2];
	int n;

	namedup = qse_wcstombsdup (name, QSE_NULL, env->mmgr); /* TODO: ignore mbwcerr */
	if (namedup == QSE_NULL) return -1;
	valuedup[0] = qse_wcsatombsdup (value, QSE_NULL, env->mmgr); /* TODO: ignore mbwcerr */
	if (valuedup == QSE_NULL)
	{
		QSE_MMGR_FREE (env->mmgr, namedup);
		return -1;
	}
	valuedup[1] = QSE_NULL;
	n = insertm (env, namedup, valuedup);
	QSE_MMGR_FREE (env->mmgr, valuedup[0]);
	QSE_MMGR_FREE (env->mmgr, namedup);

	return n;
#endif
}

static QSE_INLINE int insert_mbs (
	qse_env_t* env, const qse_mchar_t* name, const qse_mchar_t* value[])
{
#if defined(QSE_ENV_CHAR_IS_WCHAR)
	/* convert mchar to wchar */
	qse_wchar_t* namedup, * valuedup[2];
	int n;

	namedup = qse_mbstowcsalldup (name, QSE_NULL, env->mmgr); 
	if (namedup == QSE_NULL) return -1;
	valuedup[0] = qse_mbsatowcsalldup (value, QSE_NULL, env->mmgr); 
	if (valuedup[0] == QSE_NULL)
	{
		QSE_MMGR_FREE (env->mmgr, namedup);
		return -1;
	}
	valuedup[1] = QSE_NULL;
	n = insertw (env, namedup, valuedup);
	QSE_MMGR_FREE (env->mmgr, valuedup[0]);
	QSE_MMGR_FREE (env->mmgr, namedup);

	return n;
#else
	/* no conversion -> mchar */
	return insertm (env, name, value);
#endif

}

#if defined(_WIN32) 
static qse_char_t* get_env (qse_env_t* env, const qse_char_t* name, int* free)
{
	DWORD n;

	n = GetEnvironmentVariable (name, QSE_NULL, 0);
	if (n > 0) 
	{
		qse_char_t* buf;

		buf = QSE_MMGR_ALLOC (env->mmgr, n * QSE_SIZEOF(*buf));
		if (buf) 
		{
			if (GetEnvironmentVariable (name, buf, n) == n - 1)
			{
				*free = 1;
				return buf;
			}
			QSE_MMGR_FREE (env->mmgr, buf);
		}
	}

	return QSE_NULL;
}
#elif defined(QSE_ENV_CHAR_IS_WCHAR)
static qse_wchar_t* get_env (qse_env_t* env, const qse_wchar_t* name, int* free)
{
	/*
	 * This dindn't work with WATCOM C on OS2 because 
	 * _wenviron resolved to NULL.

	extern qse_wchar_t** _wenviron;
	qse_wchar_t** p = _wenviron;

	if (p)
	{
		while (*p)
		{
			qse_wchar_t* eq;
			eq = qse_wcsbeg (*p, name);
			if (eq && *eq == QSE_WT('=')) 
			{
				*free = 0;
				return eq + 1;
			}
			p++;
		}
	}
	*/

	extern char** environ;
	qse_mchar_t** p = environ;

	if (p)
	{
		while (*p)
		{
			qse_wchar_t* dup;
			qse_wchar_t* eq;

			dup = qse_mbstowcsdup (*p, env->mmgr); /* TODO: ignroe mbwcerr */
			if (dup == QSE_NULL) return QSE_NULL;

			eq = qse_wcsbeg (dup, name);
			if (eq && *eq == QSE_WT('=')) 
			{
				*free = 1;
				return eq + 1;
			}

			QSE_MMGR_FREE (env->mmgr, dup);

			p++;
		}
	}

	return 0;
}

#else

static qse_mchar_t* get_env (qse_env_t* env, const qse_mchar_t* name, int* free)
{

	#if defined(HAVE_CRT_EXTERNS_H)
	qse_mchar_t** p = *(_NSGetEnviron());
	#else
	extern char** environ;
	qse_mchar_t** p = environ;
	#endif

	if (p)
	{
		while (*p)
		{
			qse_mchar_t* eq;
			eq = qse_mbsbeg (*p, name);
			if (eq && *eq == QSE_MT('=')) 
			{
				*free = 0;
				return eq + 1;
			}
			p++;
		}
	}

	return 0;
}
#endif

static int insert_sys_wcs (qse_env_t* env, const qse_wchar_t* name)
{
#if defined(QSE_ENV_CHAR_IS_WCHAR)
	qse_wchar_t* v[2];
	int free;
	int ret = -1; 

	v[0] = get_env (env, name, &free);
	if (v[0])
	{
		v[1] = QSE_NULL;
		ret = insertw (env, name, v);
		if (free) QSE_MMGR_FREE (env->mmgr, v[0]);
	}
	return ret;
#else
	/* convert wchar to mchar */
	qse_mchar_t* namedup;
	int ret = -1;

	namedup = qse_wcstombsdup (name, QSE_NULL, env->mmgr); /* TODO: ignore mbwcerr */
	if (namedup)
	{
		ret = insert_sys_mbs (env, namedup);
		QSE_MMGR_FREE (env->mmgr, namedup);
	}

	return ret;
#endif
}

static int insert_sys_mbs (qse_env_t* env, const qse_mchar_t* name)
{
#if defined(QSE_ENV_CHAR_IS_WCHAR)
	/* convert mchar to wchar */
	qse_wchar_t* namedup;
	int ret = -1;

	namedup = qse_mbstowcsdup (name, QSE_NULL, env->mmgr); /* TODO: ignore mbwcerr */
	if (namedup)
	{
		ret = insert_sys_wcs (env, namedup);
		QSE_MMGR_FREE (env->mmgr, namedup);
	}

	return ret;
#else
	qse_mchar_t* v[2];
	int free;
	int ret = -1; 

	v[0] = get_env (env, name, &free);
	if (v[0])
	{
		v[1] = QSE_NULL;
		ret = insertm (env, name, v);
		if (free) QSE_MMGR_FREE (env->mmgr, v[0]);
	}
	return ret;
#endif
}

static int load_curenv (qse_env_t* env)
{
#if defined(_WIN32)
	qse_char_t* envstr;
	int ret = 0;

	envstr = GetEnvironmentStrings ();
	if (envstr == QSE_NULL) return -1;

#if defined(QSE_CHAR_IS_WCHAR)
	while (*envstr != QSE_WT('\0'))
	{
		/* It seems that entries like the followings exist in the 
		 * environment variable string.
		 *  - =::=::\
		 *  - =C:=C:\Documents and Settings\Administrator
		 *  - =ExitCode=00000000
		 *
		 * So entries beginning with = are excluded.
		 */
		if (*envstr != QSE_WT('=') &&
		    add_envstrw (env, envstr) <= -1) { ret = -1; goto done; }
		envstr += qse_wcslen (envstr) + 1;
	}
#else
	while (*envstr != QSE_MT('\0'))
	{
		if (*envstr != QSE_MT('=') &&
		    add_envstrm (env, envstr) <= -1) { ret = -1; goto done; }
		envstr += qse_mbslen (envstr) + 1;
	}
#endif

done:
	FreeEnvironmentStrings (envstr);
	return ret;

#elif defined(QSE_ENV_CHAR_IS_WCHAR)
	/*
	 * This dindn't work with WATCOM C on OS2 because 
	 * _wenviron resolved to NULL.
	 *
	extern qse_wchar_t** _wenviron;
	qse_wchar_t** p = _wenviron;

	if (p)
	{
		while (*p)
		{
			if (add_envstrw (env, *p) <= -1) return -1;
			p++;
		}
	}
	*/

	extern char** environ;
	qse_mchar_t** p = environ;

	if (p)
	{
		while (*p)
		{
			qse_wchar_t* dup;
			int n;

			dup = qse_mbstowcsdup (*p, env->mmgr); /* TODO: ignroe mbwcerr */
			if (dup == QSE_NULL) return -1;
			n = add_envstrw (env, dup);
			QSE_MMGR_FREE (env->mmgr, dup);
			if (n <= -1) return -1;

			p++;
		}
	}

	return 0;

#else

	#if defined(HAVE_CRT_EXTERNS_H)
	qse_mchar_t** p = *(_NSGetEnviron());
	#else
	extern char** environ;
	qse_mchar_t** p = environ;
	#endif

	if (p)
	{
		while (*p)
		{
			if (add_envstrm (env, *p) <= -1) return -1;
			p++;
		}
	}

	return 0;
#endif
}

/* ------------------------------------------------------------------- */

int qse_env_insertwcs (
	qse_env_t* env, const qse_wchar_t* name, const qse_wchar_t* value)
{
	if (value)
	{
		const qse_wchar_t* va[2];
		va[0] = value;
		va[1] = QSE_NULL;
		return insert_wcs (env, name, va);
	}
	else return insert_sys_wcs (env, name);
}

int qse_env_insertwcsa (
	qse_env_t* env, const qse_wchar_t* name, const qse_wchar_t* value[])
{
	return value? insert_wcs (env, name, value): insert_sys_wcs (env, name);
}

int qse_env_insertmbs (
	qse_env_t* env, const qse_mchar_t* name, const qse_mchar_t* value)
{
	if (value)
	{
		const qse_mchar_t* va[2];
		va[0] = value;
		va[1] = QSE_NULL;
		return insert_mbs (env, name, va);
	}
	else return insert_sys_mbs (env, name);
}

int qse_env_insertmbsa (
	qse_env_t* env, const qse_mchar_t* name, const qse_mchar_t* value[])
{
	return value? insert_mbs (env, name, value): insert_sys_mbs (env, name);
}

int qse_env_deletewcs (qse_env_t* env, const qse_wchar_t* name)
{
#if defined(QSE_ENV_CHAR_IS_WCHAR)
	return deletew (env, name);
#else
	/* convert wchar to mchar */
	qse_mchar_t* namedup;
	int n;

	namedup = qse_wcstombsdup (name, QSE_NULL, env->mmgr); /* TODO: ignore mbwcerr */
	if (namedup == QSE_NULL) return -1;

	n = deletem (env, namedup);

	QSE_MMGR_FREE (env->mmgr, namedup);
	return n;
#endif
}

int qse_env_deletembs (qse_env_t* env, const qse_mchar_t* name)
{
#if defined(QSE_ENV_CHAR_IS_WCHAR)
	/* convert mchar to wchar */
	qse_wchar_t* namedup;
	int n;

	namedup = qse_mbstowcsalldup (name, QSE_NULL, env->mmgr);
	if (namedup == QSE_NULL) return -1;

	n = deletew (env, namedup);

	QSE_MMGR_FREE (env->mmgr, namedup);
	return n;
#else
	return deletem (env, name);
#endif
}

