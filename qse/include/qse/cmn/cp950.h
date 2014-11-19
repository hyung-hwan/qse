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

#ifndef _QSE_CMN_CP950_H_
#define _QSE_CMN_CP950_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file provides functions, types, macros for cp950 conversion.
 */

/**
 * The QSE_CP950LEN_MAX macro defines the maximum number of bytes
 * needed to form a single unicode character.
 */
#define QSE_CP950LEN_MAX 2

#if defined(__cplusplus)
extern "C" {
#endif

/** 
 * The qse_uctocp950() function converts a unicode character to a cp950 sequence.
 * @return 
 * - 0 is returned if @a uc is invalid. 
 * - An integer greater than @a size is returned if the @a cp950 sequence buffer 
 *   is not #QSE_NULL and not large enough. This integer is actually the number 
 *   of bytes needed.
 * - If @a cp950 is #QSE_NULL, the number of bytes that would have been stored 
 *   into @a cp950 if it had not been #QSE_NULL is returned.
 * - An integer between 1 and size inclusive is returned in all other cases.
 * @note
 * This function doesn't check invalid unicode code points and performs
 * conversion compuationally.
 */
QSE_EXPORT qse_size_t qse_uctocp950 (
	qse_wchar_t  uc,
	qse_mchar_t* cp950,
	qse_size_t   size
);

/**
 * The qse_cp950touc() function converts a cp950 sequence to a unicode character.
 * @return
 * - 0 is returned if the @a cp950 sequence is invalid.
 * - An integer greater than @a size is returned if the @a cp950 sequence is 
 *   not complete.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */
QSE_EXPORT qse_size_t qse_cp950touc (
	const qse_mchar_t* cp950,
	qse_size_t         size, 
	qse_wchar_t*       uc
);

/**
 * The qse_cp950lenmax() function scans at most @a size bytes from the @a cp950 
 * sequence and returns the number of bytes needed to form a single unicode
 * character.
 * @return
 * - 0 is returned if the @a cp950 sequence is invalid.
 * - An integer greater than @a size is returned if the @a cp950 sequence is 
 *   not complete.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */ 
QSE_EXPORT qse_size_t qse_cp950len (
	const qse_mchar_t* cp950,
	qse_size_t         size
);

/**
 * The qse_cp950lenmax() function returns the maximum number of bytes needed 
 * to form a single unicode character. Use #QSE_CP950LEN_MAX if you need a 
 * compile-time constant.
 */
QSE_EXPORT qse_size_t qse_cp950lenmax (
	void
);

#if defined(__cplusplus)
}
#endif

#endif
