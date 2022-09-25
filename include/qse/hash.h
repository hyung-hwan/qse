/*
 * $Id: types.h 560 2011-09-06 14:18:36Z hyunghwan.chung $
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _QSE_HASH_H_
#define _QSE_HASH_H_

#include <qse/types.h>
#include <qse/macros.h>

#if (QSE_SIZEOF_SIZE_T == 4)
#	define QSE_HASH_FNV_MAGIC_INIT (0x811c9dc5)
#	define QSE_HASH_FNV_MAGIC_PRIME (0x01000193)
#elif (QSE_SIZEOF_SIZE_T == 8)

#	define QSE_HASH_FNV_MAGIC_INIT (0xCBF29CE484222325)
#	define QSE_HASH_FNV_MAGIC_PRIME (0x100000001B3l)

#elif (QSE_SIZEOF_SIZE_T == 16)
#	define QSE_HASH_FNV_MAGIC_INIT (0x6C62272E07BB014262B821756295C58D)
#	define QSE_HASH_FNV_MAGIC_PRIME (0x1000000000000000000013B)
#endif

#if defined(QSE_HASH_FNV_MAGIC_INIT)
	/* FNV-1 hash */
#	define QSE_HASH_INIT QSE_HASH_FNV_MAGIC_INIT
#	define QSE_HASH_VALUE(hv,v) (((hv) ^ (v)) * QSE_HASH_FNV_MAGIC_PRIME)

#else
	/* SDBM hash */
#	define QSE_HASH_INIT 0
#	define QSE_HASH_VALUE(hv,v) (((hv) << 6) + ((hv) << 16) - (hv) + (v))
#endif

#define QSE_HASH_VPTL(hv, ptr, len, type) do { \
	hv = QSE_HASH_INIT; \
	QSE_HASH_MORE_VPTL (hv, ptr, len, type); \
} while(0)

#define QSE_HASH_MORE_VPTL(hv, ptr, len, type) do { \
	type* __qse_hash_more_vptl_p = (type*)(ptr); \
	type* __qse_hash_more_vptl_q = (type*)__qse_hash_more_vptl_p + (len); \
	while (__qse_hash_more_vptl_p < __qse_hash_more_vptl_q) \
	{ \
		hv = QSE_HASH_VALUE(hv, *__qse_hash_more_vptl_p); \
		__qse_hash_more_vptl_p++; \
	} \
} while(0)

#define QSE_HASH_VPTR(hv, ptr, type) do { \
	hv = QSE_HASH_INIT; \
	QSE_HASH_MORE_VPTR (hv, ptr, type); \
} while(0)

#define QSE_HASH_MORE_VPTR(hv, ptr, type) do { \
	type* __qse_hash_more_vptr_p = (type*)(ptr); \
	while (*__qse_hash_more_vptr_p) \
	{ \
		hv = QSE_HASH_VALUE(hv, *__qse_hash_more_vptr_p); \
		__qse_hash_more_vptr_p++; \
	} \
} while(0)

#define QSE_HASH_BYTES(hv, ptr, len) QSE_HASH_VPTL(hv, ptr, len, const qse_uint8_t)
#define QSE_HASH_MORE_BYTES(hv, ptr, len) QSE_HASH_MORE_VPTL(hv, ptr, len, const qse_uint8_t)

#define QSE_HASH_MCHARS(hv, ptr, len) QSE_HASH_VPTL(hv, ptr, len, const qse_mchar_t)
#define QSE_HASH_MORE_MCHARS(hv, ptr, len) QSE_HASH_MORE_VPTL(hv, ptr, len, const qse_mchar_t)

#define QSE_HASH_WCHARS(hv, ptr, len) QSE_HASH_VPTL(hv, ptr, len, const qse_wchar_t)
#define QSE_HASH_MORE_WCHARS(hv, ptr, len) QSE_HASH_MORE_VPTL(hv, ptr, len, const qse_wchar_t)

#define QSE_HASH_MBS(hv, ptr) QSE_HASH_VPTR(hv, ptr, const qse_mchar_t)
#define QSE_HASH_MORE_MBS(hv, ptr) QSE_HASH_MORE_VPTR(hv, ptr, const qse_mchar_t)

#define QSE_HASH_WCS(hv, ptr) QSE_HASH_VPTR(hv, ptr, const qse_wchar_t)
#define QSE_HASH_MORE_WCS(hv, ptr) QSE_HASH_MORE_VPTR(hv, ptr, const qse_wchar_t)

#endif

