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

#ifndef _QSE_CMN_UTF8_H_
#define _QSE_CMN_UTF8_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file provides functions, types, macros for utf8 conversion.
 */

/**
 * The QSE_UTF8LEN_MAX macro defines the maximum number of bytes
 * needed to form a single unicode character.
 */
#if (QSE_SIZEOF_WCHAR_T == QSE_SIZEOF_MCHAR_T)
	/* cannot handle utf8 conversion properly */
#	define QSE_UTF8LEN_MAX 1
#elif (QSE_SIZEOF_WCHAR_T == 2)
#	define QSE_UTF8LEN_MAX 3
#elif (QSE_SIZEOF_WCHAR_T == 4)
#	define QSE_UTF8LEN_MAX 6
#else
#	error Unsupported wide-character size
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/** 
 * The qse_uctoutf8() function converts a unicode character to a utf8 sequence.
 * @return 
 * - 0 is returned if @a uc is invalid. 
 * - An integer greater than @a size is returned if the @a utf8 sequence buffer 
 *   is not #QSE_NULL and not large enough. This integer is actually the number 
 *   of bytes needed.
 * - If @a utf8 is #QSE_NULL, the number of bytes that would have been stored 
 *   into @a utf8 if it had not been #QSE_NULL is returned.
 * - An integer between 1 and size inclusive is returned in all other cases.
 * @note
 * This function doesn't check invalid unicode code points and performs
 * conversion compuationally.
 */
qse_size_t qse_uctoutf8 (
	qse_wchar_t  uc,
	qse_mchar_t* utf8,
	qse_size_t   size
);

/**
 * The qse_utf8touc() function converts a utf8 sequence to a unicode character.
 * @return
 * - 0 is returned if the @a utf8 sequence is invalid.
 * - An integer greater than @a size is returned if the @a utf8 sequence is 
 *   not complete.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */
qse_size_t qse_utf8touc (
	const qse_mchar_t* utf8,
	qse_size_t         size, 
	qse_wchar_t*       uc
);

/**
 * The qse_utf8len() function scans at most @a size bytes from the @a utf8 
 * sequence and returns the number of bytes needed to form a single unicode
 * character.
 * @return
 * - 0 is returned if the @a utf8 sequence is invalid.
 * - An integer greater than @a size is returned if the @a utf8 sequence is 
 *   not complete.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */ 
qse_size_t qse_utf8len (
	const qse_mchar_t* utf8,
	qse_size_t         size
);

/**
 * The qse_utf8lenmax() function returns the maximum number of bytes needed 
 * to form a single unicode character. Use #QSE_UTF8LEN_MAX if you need a 
 * compile-time constant.
 */
qse_size_t qse_utf8lenmax (
	void
);

#if defined(__cplusplus)
}
#endif

#endif
