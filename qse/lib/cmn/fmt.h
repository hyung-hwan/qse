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

#ifndef _QSE_LIB_CMN_FMT_H_
#define _QSE_LIB_CMN_FMT_H_

#include <qse/cmn/fmt.h>
#include <stdarg.h>

typedef int (*qse_fmtout_mchar_t) (
	qse_mchar_t c,
	void*       ctx
);

typedef int (*qse_fmtout_wchar_t) (
	qse_wchar_t c,
	void*       ctx
);

struct qse_fmtout_t
{
	qse_size_t         count;     /* out */
	qse_size_t         limit;     /* in */
	void*              ctx;       /* in */
	qse_fmtout_mchar_t put_mchar; /* in */
	qse_fmtout_wchar_t put_wchar; /* in */
};

typedef struct qse_fmtout_t qse_fmtout_t;



typedef int (*qse_mfmtout_put_t) (
	qse_mchar_t c,
	void*       ctx
);

/* convert a wide string to a multi-byte string */
typedef int (*qse_mfmtout_conv_t) (
	const qse_wchar_t* wcs,
	qse_size_t*        wcslen,
	qse_mchar_t*       mbs,
	qse_size_t*        mbslen,
	void*              ctx
);

struct qse_mfmtout_t
{
	qse_size_t         count;     /* out */
	qse_size_t         limit;     /* in */
	void*              ctx;       /* in */
	qse_mfmtout_put_t  put;       /* in */
	qse_mfmtout_conv_t conv;      /* in */
};

typedef struct qse_mfmtout_t qse_mfmtout_t;

typedef int (*qse_wfmtout_put_t) (
	qse_wchar_t c,
	void*       ctx
);

/* convert a multi-byte string to a wide string */
typedef int (*qse_wfmtout_conv_t) (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	void*              ctx
);

struct qse_wfmtout_t
{
	qse_size_t         count;     /* out */
	qse_size_t         limit;     /* in */
	void*              ctx;       /* in */
	qse_wfmtout_put_t  put;       /* in */
	qse_wfmtout_conv_t conv;      /* in */
};


typedef struct qse_wfmtout_t qse_wfmtout_t;

#if defined(__cplusplus)
extern "C" {
#endif

int qse_mfmtout (
	const qse_mchar_t* fmt,
	qse_mfmtout_t*     data,
	va_list            ap
);

int qse_wfmtout (
	const qse_wchar_t* fmt,
	qse_wfmtout_t*     data,
	va_list            ap
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_fmtout(fmt,fo,ap) qse_mfmtout(fmt,fo,ap)
#else
#	define qse_fmtout(fmt,fo,ap) qse_wfmtout(fmt,fo,ap)
#endif

#if defined(__cplusplus)
}
#endif

#endif
