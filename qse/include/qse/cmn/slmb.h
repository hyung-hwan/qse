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

#ifndef _QSE_CMN_SLMB_H_
#define _QSE_CMN_SLMB_H_

#include <qse/types.h>
#include <qse/macros.h>

/** \file
 * This file provides functions, types, macros for 
 * multibyte/wide-character conversion based on system locale.
 */

/**
 * The qse_mbstate_t type defines a structure large enough to hold
 * the standard mbstate_t.
 */
typedef struct qse_mbstate_t qse_mbstate_t;
struct qse_mbstate_t
{
#if defined(QSE_SIZEOF_MBSTATE_T) && (QSE_SIZEOF_MBSTATE_T > 0)
	qse_uint8_t dummy[QSE_SIZEOF_MBSTATE_T];
#else
	qse_uint8_t dummy[1];
#endif
};

/* --------------------------------------------------- */
/* SYSTEM LOCALE BASED CHARACTER CONVERSION            */
/* --------------------------------------------------- */

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_size_t qse_slmbrlen (
	const qse_mchar_t* mb,
	qse_size_t         mblen,
	qse_mbstate_t*     state
);

QSE_EXPORT qse_size_t qse_slmbrtoslwc (
	const qse_mchar_t* mb,
	qse_size_t         mblen,
	qse_wchar_t*       wc,
	qse_mbstate_t*     state
);

QSE_EXPORT qse_size_t qse_slwcrtoslmb (
	qse_wchar_t        wc,
	qse_mchar_t*       mb,
	qse_size_t         mblen,
	qse_mbstate_t*     state
);

/**
 * The qse_slmbtoslwc() function converts a multibyte sequence to a wide character.
 * It returns 0 if an invalid multibyte sequence is detected, mblen + 1 if the 
 * sequence is incomplete. It returns the number of bytes processed to form a 
 * wide character.
 * \note This function can not handle conversion producing non-initial
 *       states. For each call, it assumes initial state.
 */
QSE_EXPORT qse_size_t qse_slmbtoslwc (
	const qse_mchar_t* mb,
	qse_size_t         mblen,
	qse_wchar_t*       wc
);

/**
 * The qse_slwctoslmb() function converts a wide character to a multibyte sequence.
 * It returns 0 if the wide character is illegal, mblen + 1 if mblen is not 
 * large enough to hold the multibyte sequence. On successful conversion, it 
 * returns the number of bytes in the sequence.
 * \note This function can not handle conversion producing non-initial
 *       states. For each call, it assumes initial state.
 */
QSE_EXPORT qse_size_t qse_slwctoslmb (
	qse_wchar_t        wc,
	qse_mchar_t*       mb,
	qse_size_t         mblen
);

/**
 * The qse_slmblen() function scans a multibyte sequence to get the number of 
 * bytes needed to form a wide character. It does not scan more than \a mblen
 * bytes.
 * \return number of bytes processed on success, 
 *         0 for invalid sequences, 
 *         mblen + 1 for incomplete sequences
 * \note This function can not handle conversion producing non-initial
 *       states. For each call, it assumes initial state.
 */
QSE_EXPORT qse_size_t qse_slmblen (
	const qse_mchar_t* mb,
	qse_size_t         mblen
);

/**
 * The qse_slmblenmax() function returns the value of MB_CUR_MAX.
 * Note that QSE_MBLEN_MAX defines MB_LEN_MAX.
 */
QSE_EXPORT qse_size_t qse_slmblenmax (
	void
);

#if defined(__cplusplus)
}
#endif

#endif

