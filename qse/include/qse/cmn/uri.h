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

#ifndef _QSE_CMN_URI_H_
#define _QSE_CMN_URI_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_muri_t qse_muri_t;
typedef struct qse_wuri_t qse_wuri_t;

struct qse_muri_t
{
	qse_mcstr_t scheme;
	struct
	{
		qse_mcstr_t user;
		qse_mcstr_t pass;
	} auth;
	qse_mcstr_t host;
	qse_mcstr_t port;
	qse_mcstr_t path;
	qse_mcstr_t query;
	qse_mcstr_t frag;
};

struct qse_wuri_t
{
	qse_wcstr_t scheme;
	struct
	{
		qse_wcstr_t user;
		qse_wcstr_t pass;
	} auth;
	qse_wcstr_t host;
	qse_wcstr_t port;
	qse_wcstr_t path;
	qse_wcstr_t query;
	qse_wcstr_t frag;
};

enum qse_mbstouri_flag_t
{
	QSE_MBSTOURI_NOAUTH  = (1 << 0),
	QSE_MBSTOURI_NOQUERY = (1 << 1),
	QSE_MBSTOURI_NOFRAG  = (1 << 2)
};

enum qse_wcstouri_flag_t
{
	QSE_WCSTOURI_NOAUTH  = (1 << 0),
	QSE_WCSTOURI_NOQUERY = (1 << 1),
	QSE_WCSTOURI_NOFRAG  = (1 << 2)
};

#if defined(QSE_CHAR_IS_MCHAR)
#	define QSE_STRTOURI_NOAUTH  QSE_MBSTOURI_NOAUTH
#	define QSE_STRTOURI_NOQUERY QSE_MBSTOURI_NOQUERY
#	define QSE_STRTOURI_NOFRAG  QSE_MBSTOURI_NOFRAG
	typedef qse_muri_t qse_uri_t;
#else
#	define QSE_STRTOURI_NOAUTH  QSE_WCSTOURI_NOAUTH
#	define QSE_STRTOURI_NOQUERY QSE_WCSTOURI_NOQUERY
#	define QSE_STRTOURI_NOFRAG  QSE_WCSTOURI_NOFRAG
	typedef qse_wuri_t qse_uri_t;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT int qse_mbstouri (
	const qse_mchar_t* str,
	qse_muri_t*        uri,
	int                flags
);

QSE_EXPORT int qse_wcstouri (
	const qse_wchar_t* str,
	qse_wuri_t*        uri,
	int                flags
);

#if defined(QSE_CHAR_IS_MCHAR)
	#define qse_strtouri(str,uri,flags) qse_mbstouri(str,uri,flags)
#else
	#define qse_strtouri(str,uri,flags) qse_wcstouri(str,uri,flags)
#endif

#if defined(__cplusplus)
}
#endif

#endif

