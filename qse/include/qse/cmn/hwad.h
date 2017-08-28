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

#ifndef _QSE_CMN_HWAD_H_
#define _QSE_CMN_HWAD_H_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_ETHWAD_LEN 6

#include <qse/pack1.h>
struct qse_ethwad_t
{
	qse_uint8_t value[QSE_ETHWAD_LEN];
};
#include <qse/unpack.h>

typedef struct qse_ethwad_t qse_ethwad_t;

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT int qse_mbstoethwad (
	const qse_mchar_t* mbs,
	qse_ethwad_t*      hwad
);

QSE_EXPORT int qse_mbsntoethwad (
	const qse_mchar_t* mbs,
	qse_size_t         len,
	qse_ethwad_t*      hwad
);

QSE_EXPORT qse_size_t qse_ethwadtombs (
	const qse_ethwad_t* hwaddr,
	qse_mchar_t*        buf,
	qse_size_t          size
);

QSE_EXPORT int qse_wcstoethwad (
	const qse_wchar_t* mbs,
	qse_ethwad_t*      hwad
);

QSE_EXPORT int qse_wcsntoethwad (
	const qse_wchar_t* mbs,
	qse_size_t         len,
	qse_ethwad_t*      hwad
);

QSE_EXPORT qse_size_t qse_ethwadtowcs (
	const qse_ethwad_t* hwaddr,
	qse_wchar_t*        buf,
	qse_size_t          size
);

#if defined(QSE_CHAR_IS_MCHAR)
#       define qse_strtoethwad(ptr,hwad)            qse_mbstoethwad(ptr,hwad)
#       define qse_strntoethwad(ptr,len,hwad)       qse_mbsntoethwad(ptr,len,hwad)
#       define qse_ethwadtostr(hwad,ptr,len,flags)  qse_ethwadtombs(hwad,ptr,len,flags)
#else
#       define qse_strtoethwad(ptr,hwad)            qse_wcstoethwad(ptr,hwad)
#       define qse_strntoethwad(ptr,len,hwad)       qse_wcsntoethwad(ptr,len,hwad)
#       define qse_ethwadtostr(hwad,ptr,len,flags)  qse_ethwadtowcs(hwad,ptr,len,flags)
#endif

#if defined(__cplusplus)
}
#endif

#endif
