/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#ifndef _QSE_CMN_SLMB_H_
#define _QSE_CMN_SLMB_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file provides functions, types, macros for 
 * multibyte/wide-character conversion based on system locale.
 *
 * 
 */

/**
 * The qse_mbstate_t type defines a structure large enough to hold
 * the standard mbstate_t.
 */
typedef struct qse_mbstate_t qse_mbstate_t;
struct qse_mbstate_t
{
#if defined(QSE_SIZEOF_MBSTATE_T) && (QSE_SIZEOF_MBSTATE_T > 0)
	char dummy[QSE_SIZEOF_MBSTATE_T];
#else
	char dummy[1];
#endif
};

/* --------------------------------------------------- */
/* SYSTEM LOCALE BASED CHARACTER CONVERSION            */
/* --------------------------------------------------- */

#ifdef __cplusplus
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
 * @note This function can not handle conversion producing non-initial
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
 * @note This function can not handle conversion producing non-initial
 *       states. For each call, it assumes initial state.
 */
QSE_EXPORT qse_size_t qse_slwctoslmb (
	qse_wchar_t        wc,
	qse_mchar_t*       mb,
	qse_size_t         mblen
);

/**
 * The qse_slmblen() function scans a multibyte sequence to get the number of 
 * bytes needed to form a wide character. It does not scan more than @a mblen
 * bytes.
 * @return number of bytes processed on success, 
 *         0 for invalid sequences, 
 *         mblen + 1 for incomplete sequences
 * @note This function can not handle conversion producing non-initial
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

#ifdef __cplusplus
}
#endif

#endif

