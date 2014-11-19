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

#ifndef _QSE_CMN_FMT_H_
#define _QSE_CMN_FMT_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file defines various formatting functions.
 */

/** 
 * The qse_fmtintmax_flag_t type defines enumerators to change the
 * behavior of qse_fmtintmax() and qse_fmtuintmax().
 */
enum qse_fmtintmax_flag_t
{
	/* Use lower 6 bits to represent base between 2 and 36 inclusive.
	 * Upper bits are used for these flag options */

	/** Don't truncate if the buffer is not large enough */
	QSE_FMTINTMAX_NOTRUNC = (0x40 << 0),
	/** Don't append a terminating null */
	QSE_FMTINTMAX_NONULL = (0x40 << 1),
	/** Produce no digit for a value of zero  */
	QSE_FMTINTMAX_NOZERO = (0x40 << 2),
	/** Produce a leading zero for a non-zero value */
	QSE_FMTINTMAX_ZEROLEAD = (0x40 << 3),
	/** Use uppercase letters for alphabetic digits */
	QSE_FMTINTMAX_UPPERCASE = (0x40 << 4),
	/** Insert a plus sign for a positive integer including 0 */
	QSE_FMTINTMAX_PLUSSIGN = (0x40 << 5),
	/** Insert a space for a positive integer including 0 */
	QSE_FMTINTMAX_EMPTYSIGN = (0x40 << 6),
	/** Fill the right part of the string */
	QSE_FMTINTMAX_FILLRIGHT = (0x40 << 7),
	/** Fill between the sign chacter and the digit part */
	QSE_FMTINTMAX_FILLCENTER = (0x40 << 8)
};

#define QSE_FMTINTMAX_NOTRUNC         QSE_FMTINTMAX_NOTRUNC
#define QSE_FMTINTMAX_NONULL          QSE_FMTINTMAX_NONULL
#define QSE_FMTINTMAX_NOZERO          QSE_FMTINTMAX_NOZERO
#define QSE_FMTINTMAX_ZEROLEAD        QSE_FMTINTMAX_ZEROLEAD
#define QSE_FMTINTMAX_UPPERCASE       QSE_FMTINTMAX_UPPERCASE
#define QSE_FMTINTMAX_PLUSSIGN        QSE_FMTINTMAX_PLUSSIGN
#define QSE_FMTINTMAX_EMPTYSIGN       QSE_FMTINTMAX_EMPTYSIGN
#define QSE_FMTINTMAX_FILLRIGHT       QSE_FMTINTMAX_FILLRIGHT
#define QSE_FMTINTMAX_FILLCENTER      QSE_FMTINTMAX_FILLCENTER

#define QSE_FMTUINTMAX_NOTRUNC        QSE_FMTINTMAX_NOTRUNC
#define QSE_FMTUINTMAX_NONULL         QSE_FMTINTMAX_NONULL
#define QSE_FMTUINTMAX_NOZERO         QSE_FMTINTMAX_NOZERO
#define QSE_FMTUINTMAX_ZEROLEAD       QSE_FMTINTMAX_ZEROLEAD
#define QSE_FMTUINTMAX_UPPERCASE      QSE_FMTINTMAX_UPPERCASE
#define QSE_FMTUINTMAX_PLUSSIGN       QSE_FMTINTMAX_PLUSSIGN
#define QSE_FMTUINTMAX_EMPTYSIGN      QSE_FMTINTMAX_EMPTYSIGN
#define QSE_FMTUINTMAX_FILLRIGHT      QSE_FMTINTMAX_FILLRIGHT
#define QSE_FMTUINTMAX_FILLCENTER     QSE_FMTINTMAX_FILLCENTER

#define QSE_FMTINTMAXTOMBS_NOTRUNC    QSE_FMTINTMAX_NOTRUNC
#define QSE_FMTINTMAXTOMBS_NONULL     QSE_FMTINTMAX_NONULL
#define QSE_FMTINTMAXTOMBS_NOZERO     QSE_FMTINTMAX_NOZERO
#define QSE_FMTINTMAXTOMBS_ZEROLEAD   QSE_FMTINTMAX_ZEROLEAD
#define QSE_FMTINTMAXTOMBS_UPPERCASE  QSE_FMTINTMAX_UPPERCASE
#define QSE_FMTINTMAXTOMBS_PLUSSIGN   QSE_FMTINTMAX_PLUSSIGN
#define QSE_FMTINTMAXTOMBS_EMPTYSIGN  QSE_FMTINTMAX_EMPTYSIGN
#define QSE_FMTINTMAXTOMBS_FILLRIGHT  QSE_FMTINTMAX_FILLRIGHT
#define QSE_FMTINTMAXTOMBS_FILLCENTER QSE_FMTINTMAX_FILLCENTER

#define QSE_FMTUINTMAXTOMBS_NOTRUNC    QSE_FMTINTMAX_NOTRUNC
#define QSE_FMTUINTMAXTOMBS_NONULL     QSE_FMTINTMAX_NONULL
#define QSE_FMTUINTMAXTOMBS_NOZERO     QSE_FMTINTMAX_NOZERO
#define QSE_FMTUINTMAXTOMBS_ZEROLEAD   QSE_FMTINTMAX_ZEROLEAD
#define QSE_FMTUINTMAXTOMBS_UPPERCASE  QSE_FMTINTMAX_UPPERCASE
#define QSE_FMTUINTMAXTOMBS_PLUSSIGN   QSE_FMTINTMAX_PLUSSIGN
#define QSE_FMTUINTMAXTOMBS_EMPTYSIGN  QSE_FMTINTMAX_EMPTYSIGN
#define QSE_FMTUINTMAXTOMBS_FILLRIGHT  QSE_FMTINTMAX_FILLRIGHT
#define QSE_FMTUINTMAXTOMBS_FILLCENTER QSE_FMTINTMAX_FILLCENTER

#define QSE_FMTINTMAXTOWCS_NOTRUNC    QSE_FMTINTMAX_NOTRUNC
#define QSE_FMTINTMAXTOWCS_NONULL     QSE_FMTINTMAX_NONULL
#define QSE_FMTINTMAXTOWCS_NOZERO     QSE_FMTINTMAX_NOZERO
#define QSE_FMTINTMAXTOWCS_ZEROLEAD   QSE_FMTINTMAX_ZEROLEAD
#define QSE_FMTINTMAXTOWCS_UPPERCASE  QSE_FMTINTMAX_UPPERCASE
#define QSE_FMTINTMAXTOWCS_PLUSSIGN   QSE_FMTINTMAX_PLUSSIGN
#define QSE_FMTINTMAXTOWCS_EMPTYSIGN  QSE_FMTINTMAX_EMPTYSIGN
#define QSE_FMTINTMAXTOWCS_FILLRIGHT  QSE_FMTINTMAX_FILLRIGHT
#define QSE_FMTINTMAXTOWCS_FILLCENTER QSE_FMTINTMAX_FILLCENTER

#define QSE_FMTUINTMAXTOWCS_NOTRUNC    QSE_FMTINTMAX_NOTRUNC
#define QSE_FMTUINTMAXTOWCS_NONULL     QSE_FMTINTMAX_NONULL
#define QSE_FMTUINTMAXTOWCS_NOZERO     QSE_FMTINTMAX_NOZERO
#define QSE_FMTUINTMAXTOWCS_ZEROLEAD   QSE_FMTINTMAX_ZEROLEAD
#define QSE_FMTUINTMAXTOWCS_UPPERCASE  QSE_FMTINTMAX_UPPERCASE
#define QSE_FMTUINTMAXTOWCS_PLUSSIGN   QSE_FMTINTMAX_PLUSSIGN
#define QSE_FMTUINTMAXTOWCS_EMPTYSIGN  QSE_FMTINTMAX_EMPTYSIGN
#define QSE_FMTUINTMAXTOWCS_FILLRIGHT  QSE_FMTINTMAX_FILLRIGHT
#define QSE_FMTUINTMAXTOWCS_FILLCENTER QSE_FMTINTMAX_FILLCENTER

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_fmtintmaxtombs() function formats an integer @a value to a 
 * multibyte string according to the given base and writes it to a buffer 
 * pointed to by @a buf. It writes to the buffer at most @a size characters 
 * including the terminating null. The base must be between 2 and 36 inclusive 
 * and can be ORed with zero or more #qse_fmtintmaxtombs_flag_t enumerators. 
 * This ORed value is passed to the function via the @a base_and_flags 
 * parameter. If the formatted string is shorter than @a bufsize, the redundant
 * slots are filled with the fill character @a fillchar if it is not a null 
 * character. The filling behavior is determined by the flags shown below:
 *
 * - If #QSE_FMTINTMAXTOMBS_FILLRIGHT is set in @a base_and_flags, slots 
 *   after the formatting string are filled.
 * - If #QSE_FMTINTMAXTOMBS_FILLCENTER is set in @a base_and_flags, slots 
 *   before the formatting string are filled. However, if it contains the
 *   sign character, the slots between the sign character and the digit part
 *   are filled.  
 * - If neither #QSE_FMTINTMAXTOMBS_FILLRIGHT nor #QSE_FMTINTMAXTOMBS_FILLCENTER
 *   , slots before the formatting string are filled.
 *
 * The @a precision parameter specified the minimum number of digits to
 * produce from the @ value. If @a value produces fewer digits than
 * @a precision, the actual digits are padded with '0' to meet the precision
 * requirement. You can pass a negative number if you don't wish to specify
 * precision.
 *
 * The terminating null is not added if #QSE_FMTINTMAXTOMBS_NONULL is set;
 * The #QSE_FMTINTMAXTOMBS_UPPERCASE flag indicates that the function should
 * use the uppercase letter for a alphabetic digit; 
 * You can set #QSE_FMTINTMAXTOMBS_NOTRUNC if you require lossless formatting.
 * The #QSE_FMTINTMAXTOMBS_PLUSSIGN flag and #QSE_FMTINTMAXTOMBS_EMPTYSIGN 
 * ensures that the plus sign and a space is added for a positive integer 
 * including 0 respectively.
 * The #QSE_FMTINTMAXTOMBS_ZEROLEAD flag ensures that the numeric string
 * begins with '0' before applying the prefix.
 * You can set the #QSE_FMTINTMAXTOMBS_NOZERO flag if you want the value of
 * 0 to produce nothing. If both #QSE_FMTINTMAXTOMBS_NOZERO and 
 * #QSE_FMTINTMAXTOMBS_ZEROLEAD are specified, '0' is still produced.
 * 
 * If @a prefix is not #QSE_NULL, it is inserted before the digits.
 * 
 * @return
 *  - -1 if the base is not between 2 and 36 inclusive. 
 *  - negated number of characters required for lossless formatting 
 *   - if @a bufsize is 0.
 *   - if #QSE_FMTINTMAXTOMBS_NOTRUNC is set and @a bufsize is less than
 *     the minimum required for lossless formatting.
 *  - number of characters written to the buffer excluding a terminating 
 *    null in all other cases.
 */
QSE_EXPORT int qse_fmtintmaxtombs (
	qse_mchar_t*       buf,             /**< buffer pointer */
	int                bufsize,         /**< buffer size */
	qse_intmax_t       value,           /**< integer to format */
	int                base_and_flags,  /**< base ORed with flags */
	int                precision,       /**< precision */
	qse_mchar_t        fillchar,        /**< fill character */
	const qse_mchar_t* prefix           /**< prefix */
);

/**
 * The qse_fmtintmaxtowcs() function formats an integer @a value to a 
 * wide-character string according to the given base and writes it to a buffer 
 * pointed to by @a buf. It writes to the buffer at most @a size characters 
 * including the terminating null. The base must be between 2 and 36 inclusive 
 * and can be ORed with zero or more #qse_fmtintmaxtowcs_flag_t enumerators. 
 * This ORed value is passed to the function via the @a base_and_flags 
 * parameter. If the formatted string is shorter than @a bufsize, the redundant
 * slots are filled with the fill character @a fillchar if it is not a null 
 * character. The filling behavior is determined by the flags shown below:
 *
 * - If #QSE_FMTINTMAXTOWCS_FILLRIGHT is set in @a base_and_flags, slots 
 *   after the formatting string are filled.
 * - If #QSE_FMTINTMAXTOWCS_FILLCENTER is set in @a base_and_flags, slots 
 *   before the formatting string are filled. However, if it contains the
 *   sign character, the slots between the sign character and the digit part
 *   are filled.  
 * - If neither #QSE_FMTINTMAXTOWCS_FILLRIGHT nor #QSE_FMTINTMAXTOWCS_FILLCENTER
 *   , slots before the formatting string are filled.
 * 
 * The @a precision parameter specified the minimum number of digits to
 * produce from the @ value. If @a value produces fewer digits than
 * @a precision, the actual digits are padded with '0' to meet the precision
 * requirement. You can pass a negative number if don't wish to specify
 * precision.
 *
 * The terminating null is not added if #QSE_FMTINTMAXTOWCS_NONULL is set;
 * The #QSE_FMTINTMAXTOWCS_UPPERCASE flag indicates that the function should
 * use the uppercase letter for a alphabetic digit; 
 * You can set #QSE_FMTINTMAXTOWCS_NOTRUNC if you require lossless formatting.
 * The #QSE_FMTINTMAXTOWCS_PLUSSIGN flag and #QSE_FMTINTMAXTOWCS_EMPTYSIGN 
 * ensures that the plus sign and a space is added for a positive integer 
 * including 0 respectively.
 * The #QSE_FMTINTMAXTOWCS_ZEROLEAD flag ensures that the numeric string
 * begins with 0 before applying the prefix.
 * You can set the #QSE_FMTINTMAXTOWCS_NOZERO flag if you want the value of
 * 0 to produce nothing. If both #QSE_FMTINTMAXTOWCS_NOZERO and 
 * #QSE_FMTINTMAXTOWCS_ZEROLEAD are specified, '0' is still produced.
 *
 * If @a prefix is not #QSE_NULL, it is inserted before the digits.
 * 
 * @return
 *  - -1 if the base is not between 2 and 36 inclusive. 
 *  - negated number of characters required for lossless formatting 
 *   - if @a bufsize is 0.
 *   - if #QSE_FMTINTMAXTOWCS_NOTRUNC is set and @a bufsize is less than
 *     the minimum required for lossless formatting.
 *  - number of characters written to the buffer excluding a terminating 
 *    null in all other cases.
 */
QSE_EXPORT int qse_fmtintmaxtowcs (
	qse_wchar_t*       buf,             /**< buffer pointer */
	int                bufsize,         /**< buffer size */
	qse_intmax_t       value,           /**< integer to format */
	int                base_and_flags,  /**< base ORed with flags */
	int                precision,       /**< precision */
	qse_wchar_t        fillchar,        /**< fill character */
	const qse_wchar_t* prefix           /**< prefix */
);

/** @def qse_fmtintmax
 * The qse_fmtintmax() macro maps to qse_fmtintmaxtombs() if 
 * #QSE_CHAR_IS_MCHAR, and qse_fmtintmaxtowcs() if #QSE_CHAR_IS_WCHAR.
 */
#ifdef QSE_CHAR_IS_MCHAR
#	define qse_fmtintmax(b,sz,v,bf,pr,fc,pf) qse_fmtintmaxtombs(b,sz,v,bf,pr,fc,pf)
#else
#	define qse_fmtintmax(b,sz,v,bf,pr,fc,pf) qse_fmtintmaxtowcs(b,sz,v,bf,pr,fc,pf)
#endif

/**
 * The qse_fmtuintmaxtombs() function formats an unsigned integer @a value 
 * to a multibyte string buffer. It behaves the same as qse_fmtuintmaxtombs() 
 * except that it handles an unsigned integer.
 */
QSE_EXPORT int qse_fmtuintmaxtombs (
	qse_mchar_t*       buf,             /**< buffer pointer */
	int                bufsize,         /**< buffer size */
	qse_uintmax_t      value,           /**< integer to format */
	int                base_and_flags,  /**< base ORed with flags */
	int                precision,       /**< precision */
	qse_mchar_t        fillchar,        /**< fill character */
	const qse_mchar_t* prefix           /**< prefix */
);

/**
 * The qse_fmtuintmaxtowcs() function formats an unsigned integer @a value 
 * to a wide-character string buffer. It behaves the same as 
 * qse_fmtuintmaxtowcs() except that it handles an unsigned integer.
 */
QSE_EXPORT int qse_fmtuintmaxtowcs (
	qse_wchar_t*       buf,             /**< buffer pointer */
	int                bufsize,         /**< buffer size */
	qse_uintmax_t      value,           /**< integer to format */
	int                base_and_flags,  /**< base ORed with flags */
	int                precision,       /**< precision */
	qse_wchar_t        fillchar,        /**< fill character */
	const qse_wchar_t* prefix           /**< prefix */
);

/** @def qse_fmtuintmax
 * The qse_fmtuintmax() macro maps to qse_fmtuintmaxtombs() if 
 * #QSE_CHAR_IS_MCHAR, and qse_fmtuintmaxtowcs() if #QSE_CHAR_IS_WCHAR.
 */
#ifdef QSE_CHAR_IS_MCHAR
#	define qse_fmtuintmax(b,sz,v,bf,pr,fc,pf) qse_fmtuintmaxtombs(b,sz,v,bf,pr,fc,pf)
#else
#	define qse_fmtuintmax(b,sz,v,bf,pr,fc,pf) qse_fmtuintmaxtowcs(b,sz,v,bf,pr,fc,pf)
#endif

QSE_EXPORT int qse_fmtfltmaxtombs (
	qse_mchar_t* buf, 
	qse_size_t   bufsize,
	qse_fltmax_t f,
	qse_mchar_t  point, 
	int          digits
);

#if defined(__cplusplus)
}
#endif

#endif
