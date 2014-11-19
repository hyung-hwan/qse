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

#ifndef _QSE_CMN_MB8_H_
#define _QSE_CMN_MB8_H_

#include <qse/types.h>
#include <qse/macros.h>

/** \file
 * This file provides functions, types, macros for mb8 conversion.
 */

#if defined(__cplusplus)
extern "C" {
#endif

/** 
 * The qse_wctomb8() function converts a wide character to a mb8 sequence.
 * \return 
 * - 0 is returned if \a wc is invalid. 
 * - An integer greater than \a size is returned if the \a mb8 sequence buffer 
 *   is not #QSE_NULL and not large enough. This integer is actually the number 
 *   of bytes needed.
 * - If \a mb8 is #QSE_NULL, the number of bytes that would have been stored 
 *   into \a mb8 if it had not been #QSE_NULL is returned.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */
qse_size_t qse_wctomb8 (
	qse_wchar_t  wc,
	qse_mchar_t* mb8,
	qse_size_t   size
);

/**
 * The qse_mb8towc() function converts a mb8 sequence to a wide character.
 * \return
 * - 0 is returned if the \a mb8 sequence is invalid.
 * - An integer greater than \a size is returned if the \a mb8 sequence is 
 *   not complete.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */
qse_size_t qse_mb8towc (
	const qse_mchar_t* mb8,
	qse_size_t         size, 
	qse_wchar_t*       wc
);

#if defined(__cplusplus)
}
#endif

#endif
