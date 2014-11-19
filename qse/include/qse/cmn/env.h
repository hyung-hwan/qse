/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_CMN_ENV_H_
#define _QSE_CMN_ENV_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file defines data types and functions that you can use to build
 * an environment block. 
 */

/* 
 * Note:
 * The wprintf function provided by Watcom C doesn't seem to be able to 
 * print multibyte-characters properly at least on OS2. You may have 
 * difficulty if you try to print the environment strings with Watcom C.
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
	qse_mmgr_t* mmgr;

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


#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_env_t* qse_env_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize,
	int         fromcurenv
);

QSE_EXPORT void qse_env_close (
	qse_env_t* env
);

QSE_EXPORT int qse_env_init (
	qse_env_t*  env,
	qse_mmgr_t* mmgr,
	int         fromcurenv
);

QSE_EXPORT void qse_env_fini (
	qse_env_t* env
);

QSE_EXPORT qse_mmgr_t* qse_env_getmmgr (
	qse_env_t* env
);

QSE_EXPORT void* qse_env_getxtn (
	qse_env_t* env
);

QSE_EXPORT void qse_env_clear (
	qse_env_t* env
);

QSE_EXPORT const qse_env_char_t* qse_env_getstr (
	qse_env_t* env
);

QSE_EXPORT qse_env_char_t** qse_env_getarr (
	qse_env_t* env
);

/**
 * The qse_env_insertwcs() function adds a new environment variable
 * @a name with the @a value. If the @a value is #QSE_NULL, it takes
 * the actual value from the system environment 
 *
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_env_insertwcs (
	qse_env_t*         env,
	const qse_wchar_t* name,
	const qse_wchar_t* value
);

QSE_EXPORT int qse_env_insertwcsa (
	qse_env_t*         env,
	const qse_wchar_t* name,
	const qse_wchar_t* value[]
);

QSE_EXPORT int qse_env_insertmbs (
	qse_env_t*         env,
	const qse_mchar_t* name,
	const qse_mchar_t* value
);

QSE_EXPORT int qse_env_insertmbsa (
	qse_env_t*         env,
	const qse_mchar_t* name,
	const qse_mchar_t* value[]
);

/**
 * The qse_env_appendwcs() function appends an extra value to the last item
 * in the environment list.
 */
QSE_EXPORT int qse_env_appendwcs (
	qse_env_t*         env,
	const qse_wchar_t* value
);

QSE_EXPORT int qse_env_appendmbs (
	qse_env_t*         env,
	const qse_mchar_t* value
);

QSE_EXPORT int qse_env_deletewcs (
	qse_env_t*         env,
	const qse_wchar_t* name
);

QSE_EXPORT int qse_env_deletembs (
	qse_env_t*         env,
	const qse_mchar_t* name
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_env_insert(env,name,value) qse_env_insertmbs(env,name,value)
#	define qse_env_inserta(env,name,value) qse_env_insertmbsa(env,name,value)
#	define qse_env_append(env,value) qse_env_appendmbs(env,value)
#	define qse_env_delete(env,name) qse_env_deletembs(env,name)
#else
#	define qse_env_insert(env,name,value) qse_env_insertwcs(env,name,value)
#	define qse_env_inserta(env,name,value) qse_env_insertwcsa(env,name,value)
#	define qse_env_append(env,value) qse_env_appendwcs(env,value)
#	define qse_env_delete(env,name) qse_env_deletewcs(env,name)
#endif

#if defined(__cplusplus)
}
#endif

#endif
