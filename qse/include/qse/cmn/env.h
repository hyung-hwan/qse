/*
 * $Id$
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

/** @file
 * This file defines data types and functions that you can use to build
 * an environment block. 
 */


#if defined(_WIN32) && defined(QSE_CHAR_IS_WCHAR)
	typedef qse_wchar_t qse_env_char_t;
#	define QSE_ENV_CHAR_IS_WCHAR
#else
	typedef qse_mchar_t qse_env_char_t;
#	define QSE_ENV_CHAR_IS_MCHAR
#endif

/**
 * The qse_env_t type defines a cross-platform environment block.
 */
typedef struct qse_env_t qse_env_t;

struct qse_env_t
{
	QSE_DEFINE_COMMON_FIELDS(env)

	struct
	{
		qse_size_t  capa;
		qse_size_t  len;
		qse_env_char_t* ptr;
	} str;

	struct
	{
		qse_size_t   capa;
		qse_size_t   len;
		qse_env_char_t** ptr;
	} arr;
};


#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS(env)

qse_env_t* qse_env_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize,
	int         fromcurenv
);

void qse_env_close (
	qse_env_t* env
);

qse_env_t* qse_env_init (
	qse_env_t*  env,
	qse_mmgr_t* mmgr,
	int         fromcurenv
);

void qse_env_fini (
	qse_env_t* env
);

void qse_env_clear (
	qse_env_t* env
);

#define qse_env_getstr(env) ((env)->str.ptr)
#define qse_env_getarr(env) ((env)->arr.ptr)

int qse_env_insertw (
	qse_env_t*        env,
	const qse_wchar_t* name,
	const qse_wchar_t* value
);

int qse_env_insertm (
	qse_env_t*        env,
	const qse_mchar_t* name,
	const qse_mchar_t* value
);

int qse_env_deletew (
	qse_env_t*        env,
	const qse_wchar_t* name
);

int qse_env_deletem (
	qse_env_t*        env,
	const qse_mchar_t* name
);

#if defined(QSE_CHAR_IS_WCHAR)
#	define qse_env_insert(env,name,value) qse_env_insertw(env,name,value)
#	define qse_env_delete(env,name) qse_env_deletew(env,name)
#else
#	define qse_env_insert(env,name,value) qse_env_insertm(env,name,value)
#	define qse_env_delete(env,name) qse_env_deletem(env,name)
#endif

#ifdef __cplusplus
}
#endif

#endif
